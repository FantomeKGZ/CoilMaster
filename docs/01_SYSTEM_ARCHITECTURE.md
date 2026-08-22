# CoilMaster — системная архитектура

Ветка реализации/source-of-truth: `cmp-protocol-v1`.

## Граница ответственности

### Arduino Uno — realtime controller

Arduino владеет физически опасной частью станка:

- physical/local START;
- SSR;
- Hall/подсчёт витков;
- keypad/LCD/buzzer;
- realtime state machine;
- выполнение уже принятого remote JOB;
- `RUN_STARTED` / `RUN_COMPLETED` события.

ESP32/Web не имеют прямого пути управления SSR. Между повторениями нет automatic physical START. После reboot нет automatic winding resume.

### ESP32 — service/data controller

ESP32 отвечает за:

- Wi-Fi/AP, mDNS, HTTP и ограниченный recovery FTP;
- microSD и DS3231 RTC;
- desktop/mobile Web UI и API;
- клиентов, двигатели и ремонты;
- costing, warehouse/materials и ручное списание;
- подготовку/доставку JOB Arduino;
- event journal и recovery evidence;
- backup/restore;
- diagnostics и безопасные hardware settings.

## Production flow

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> immutable snapshot (+ exact spool selection when a spool is used)
-> local CREATED preparation -> DELIVERING -> UART
-> Arduino READY -> physical START
-> RUN_STARTED -> RUN_COMPLETED
-> manual exact-run material writeoff
-> costing/finalization preflight -> CLOSED -> reports -> backup
```

`RUN_COMPLETED` никогда сам по себе не списывает провод.

Manual writeoff всегда привязан к exact:

```text
source_session_id + source_run_id
```

`spool_id` обязателен, если используется конкретная катушка. В утверждённом KG_FIRST unallocated/manual path `spool_id` может отсутствовать; если spool использовался, его provenance сохраняется точно.

## Production UART

Рабочий межконтроллерный транспорт — текстовый `CMP1|...` с CRC16 и строгим разбором полей:

```text
Arduino/CM_UartEventTransport.*
firmware/esp32/src/CM_UartEventReceiver.*
Shared/CMP1Text/CM_Cmp1Crc.h
```

`Shared/Protocol/` содержит отдельный ранний binary CMP и используется host-tests. Это не production wire layer. Его назначение документировано в `Shared/Protocol/README.md`.

Потеря ACK или `TIMED_OUT` сама по себе никогда не доказывает, что Arduino idle. Late real `RUN_STARTED` для exact session/run reconciliation обрабатывается отдельно.

## Хранение и crash consistency

Authoritative persistent data хранится на microSD под `/data`; web assets — под `/web`.

Основные правила:

- production mutable single-file replacement использует temp/backup/verify/commit semantics;
- immutable winding snapshot/spool provenance не удаляется автоматическим cleanup;
- torn/malformed production NDJSON переводит соответствующий owner в fail-closed состояние;
- автоматическое усечение/удаление production evidence запрещено;
- restore operator-only, transactional и fail-closed;
- reboot не продолжает restore/apply автоматически;
- production HTTP mutations блокируются на время restore apply/rollback interlock.

## Network/recovery

AP recovery поднимается до попытки STA. Ошибка persisted network profile не должна лишать оператора recovery AP.

FTP ограничен recovery web-root и не является общим файловым доступом к `/data`.

## Build boundaries

`platformio.ini` — authoritative build-input source.

Arduino Uno production build:

```text
Core/*.cpp
Arduino/*.cpp
firmware/arduino/src/main.cpp
```

ESP32 production build:

```text
firmware/esp32/src/*.cpp
```

Web assets:

```text
firmware/esp32/web/
```

## Hardware interfaces

Точные wiring/pin данные должны браться из `docs/HARDWARE_REFERENCE/` и текущего firmware config, а не из старых architecture drafts.

## Verification boundary

CI/build не заменяет targeted two-board hardware smoke. Hardware GREEN не выводится из software CI.

Текущие status/active work:

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```
