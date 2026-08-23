# CoilMaster — ESP32 Core

## Назначение

ESP32 является информационным, сетевым и сервисным контроллером. Она хранит production-данные, обслуживает Web/API и связывает бизнес-состояние с фактическими событиями Arduino, но не заменяет Arduino в критической логике управления станком.

## Ответственность

- локальный Wi‑Fi Web-сервер;
- Web UI и API;
- UART/CMP1 с Arduino Uno;
- RTC DS3231;
- microSD и persisted production state;
- базы клиентов, двигателей, ремонтов, обмоток и склада;
- immutable job snapshot/spool selection;
- журналирование;
- резервное копирование и restore safety gates;
- диагностика;
- обработка delivery/cancel/recovery состояния задания.

## Safety boundary

ESP32/Web:

- не управляют SSR напрямую;
- не инициируют physical START;
- не выполняют automatic resume после reboot;
- не считают `RUN_COMPLETED` разрешением на automatic wire writeoff.

Физический START и фактические `RUN_STARTED` / `RUN_COMPLETED` принадлежат Arduino.

## Production flow

```text
client
→ motor
→ OPEN repair
→ costing
→ linked winding
→ exact spool
→ immutable snapshot/spool selection
→ UART JOB
→ JOB_ACK
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ manual exact-run exact-spool wire writeoff
→ costing
→ finalization preflight
→ CLOSED
→ reports
→ backup
```

## Доставка задания

ESP32 формирует `JOB` с уникальным `job_id`, `session_id`, типом обмотки и программой витков. До положительного `JOB_ACK` persisted state остается в delivery-состоянии.

Retry ограничен по количеству попыток. Потеря `JOB_ACK` рассматривается как неопределенность: кадр мог физически попасть на Arduino.

Поэтому операторская отмена pending delivery работает так:

1. ESP32 прекращает retransmit `JOB`;
2. если JOB еще точно не отправлялся — допускается локальное закрытие delivery;
3. если хотя бы один JOB мог попасть на Arduino — ESP32 переходит в `JOB_CANCEL` handshake;
4. persisted state закрывается только после безопасного результата отмены.

Это исключает ghost-job, когда ESP32 считает задание отмененным, а Arduino продолжает хранить его в `Ready`.

## Отмена принятого задания

No-run задание может быть отменено и после `JOB_ACK ACCEPTED`, в том числе если оно связано с repair/motor/spool snapshot.

Связанные immutable metadata не удаляются. Разрешена только отмена operational job state до physical START.

Отмена запрещена при наличии физического run evidence:

- активный run;
- `last_run_id != 0`;
- `completed_runs != 0`.

## Recovery после рассинхронизации

Arduino имеет физический fallback `D → * → # → D`. При безопасной очистке он отправляет:

```text
CMP1|JOB_CANCEL_ACK|0|CANCELLED|ALL_CLEAR|C|<CRC>
```

ESP32:

- принимает только валидный CRC frame;
- сопоставляет `ALL_CLEAR` с pending или восстановленным persisted job;
- очищает transient UART retry state;
- публикует обычный `JobCancelResult::Cancelled` с detail `ALL_CLEAR`;
- закрывает persisted job через тот же audited cancellation path.

После reboot ESP32 восстанавливает идентификатор незакрытого задания и передает его в UART receiver через remembered job correlation. Это позволяет физическому `ALL_CLEAR` закрыть recovery-неопределенность без подмены результата намотки.

## Persisted job lifecycle

Положительная remote cancellation может закрыть no-run состояния:

- `Created/Delivering + WaitingDelivery`;
- `Accepted + WaitingPhysicalStart`;
- восстановленное неопределенное no-run состояние, если Arduino подтверждает clear.

Она не может закрыть job с run evidence.

## Hall hardware-control lane

Для настройки Hall используется отдельный `CM_HardwareControlClient`, а физический UART остаётся общим с winding protocol.

Архитектурные правила:

- `CM_UartEventReceiver` остается единственным reader байтов UART;
- Hall/config frames делегируются в hardware-control client до winding parser;
- JOB/JOB_CANCEL и Hall control используют одну взаимно исключающую control lane;
- один hardware request активен за раз;
- retry bounded; отсутствие ответа завершается явным `TimedOut`, а не положительным предположением;
- settings/telemetry state сохраняют ESP32 `receivedAtMs`, чтобы HTTP/UI различали fresh/stale данные;
- hardware-control frames не выполняют physical START и не управляют SSR.

Current HTTP/Web owner уже реализован:

```text
firmware/esp32/src/CM_HardwareControlWeb.h/.cpp
firmware/esp32/web/desktop/settings-hall.html
firmware/esp32/web/mobile/settings-hall.html
firmware/esp32/web/shared/settings-hall-calibration.js
```

Current route family включает Hall state/settings, telemetry и calibration (`refresh/arm/abort`). `calibration/arm` означает только подготовку Arduino calibration state; фактическое движение по-прежнему требует physical START на Arduino.

Web direct SSR test/control API отсутствует и не должен добавляться как shortcut.

Targeted safety contracts находятся в `Tests/Web/check_hall_calibration_contracts.js` и related protocol/build gates.

## Wire writeoff

`RUN_COMPLETED` — только фактическое событие Arduino. Списание провода остается ручным и привязывается к точному:

```text
spool_id
source_session_id
source_run_id
```

Для current linked production `spool_id` обязан совпадать с immutable pre-UART spool selection; historical `UNALLOCATED` evidence не создает optional-spool fallback.

## Хранилище

Рабочие данные на microSD должны сохранять fail-closed semantics. Для критических persisted transitions используются валидируемые записи и recovery/audit механизмы. Backup restore остается operator-only, transactional и без auto-resume после reboot.

Различные job temp/residue stores имеют разные transaction boundaries; их нельзя автоматически унифицировать одной cleanup/recovery политикой.
