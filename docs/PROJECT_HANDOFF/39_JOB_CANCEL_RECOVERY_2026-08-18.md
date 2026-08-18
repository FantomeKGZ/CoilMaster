# CoilMaster — JOB cancel / recovery hardening

Дата: **2026-08-18**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Source of truth: `cmp-protocol-v1`

## Почему появился этот checkpoint

После release-ready checkpoint `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md` обнаружен реальный operational edge case:

- ESP32 отправила winding JOB;
- задание не появилось на Arduino или ACK был потерян;
- ESP32 сохраняла pending/uncertain job;
- оператор не мог надежно сбросить это состояние обычной отменой.

Это означало возможную рассинхронизацию: ESP32 не знает, дошел ли JOB, а Arduino может иметь или не иметь remote job.

## Реализованный fix

Production firmware изменён после старого release baseline.

Ключевой functional commit:

```text
7d8bc93fdcd626f358dd1baa22428b8447355b2d
fix: make ESP32 Arduino job cancellation resilient
```

Follow-up для linked no-run jobs:

```text
1c66938cd52ce790b9833faf93fc647b5bae5725
fix: allow safe cancellation of linked no-run jobs
```

### ESP32 pending JOB cancellation

`UartEventReceiver::cancelPendingJob()` теперь различает:

- JOB еще точно не отправлялся → допустима локальная cancellation;
- JOB мог хотя бы один раз попасть на Arduino → ESP32 прекращает JOB retransmit и переключается на `JOB_CANCEL` handshake.

Это закрывает ghost-job сценарий при потерянном `JOB_ACK`.

### Web/API cancel semantics

No-run job теперь можно отменить как в состоянии pending delivery, так и после `JOB_ACK ACCEPTED`, если physical START еще не происходил.

Linked repair/motor/spool metadata не являются причиной запирать такое задание. Immutable snapshot/history сохраняются.

Отмена остается запрещенной при физическом run evidence:

- active run;
- `last_run_id != 0`;
- `completed_runs != 0`.

### Arduino idempotent cancel

Arduino обрабатывает repeated/late `JOB_CANCEL` идемпотентно:

- exact Ready remote job без run evidence → cancel;
- remote job уже отсутствует → successful already-clear semantics;
- другой/busy remote job → reject.

Это предотвращает бесконечный reject/retry после reboot или уже выполненной очистки.

## Физический emergency recovery

На Arduino добавлена последовательность:

```text
D → * → # → D
```

Она предназначена только для восстановления синхронизации ESP32 ↔ Arduino.

При безопасной очистке Arduino отправляет CRC-protected frame:

```text
CMP1|JOB_CANCEL_ACK|0|CANCELLED|ALL_CLEAR|C|<CRC>
```

### Что означает ALL_CLEAR

`ALL_CLEAR` означает только:

> Arduino физически не удерживает активный remote job, который можно продолжить как pending no-run assignment.

Это **не** означает:

- `RUN_COMPLETED`;
- завершение ремонта;
- выполненную намотку;
- списание провода;
- разрешение на SSR/START.

### Блокировки fallback

Emergency clear не должен очищать remote job при активной/paused намотке или наличии run evidence.

Если remote job отсутствует, локальная standalone работа не должна удаляться этим recovery path.

## Reboot correlation

ESP32 сохраняет/восстанавливает идентификатор активного persisted job в UART receiver (`rememberJobId`). Поэтому `ALL_CLEAR` с `job_id=0` может быть безопасно сопоставлен с recovered uncertainty после reboot.

Положительный ALL_CLEAR проходит через обычный `JobCancelResult::Cancelled` и persisted cancellation path, а не через специальный bypass.

## Persisted cancellation semantics

Положительная remote cancellation может закрыть no-run состояния:

- `Created/Delivering + WaitingDelivery`;
- `Accepted + WaitingPhysicalStart`;
- recovered no-run uncertainty, если Arduino подтверждает clear.

Job с run evidence таким путем не закрывается.

## Safety invariants не изменены

- никакого automatic physical START;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
- `RUN_COMPLETED` не списывает провод автоматически;
- wire writeoff остается manual и требует exact `spool_id + source_session_id + source_run_id`;
- linked immutable snapshot/spool history не удаляется отменой operational no-run job.

## Verification

One-shot GitHub Actions verifier перед final fix выполнил fail-fast последовательность:

```text
cmake -S Tests/Protocol -B build/cmp-protocol
cmake --build build/cmp-protocol
ctest --test-dir build/cmp-protocol
node Tests/Web/check_web_assets.js
node Tests/Web/check_release_contracts.js
node Tests/Web/check_final_acceptance_contracts.js
pio run -e uno
pio run -e esp32
```

Final bot commit `1c66938cd52ce790b9833faf93fc647b5bae5725` был создан только после успешного прохождения этих команд.

Поэтому подтверждено для этого change-set:

- host CMP tests passed;
- web asset/release/final-acceptance contract checks passed;
- Arduino Uno build passed;
- ESP32 build passed.

Это не заменяет новый hardware regression после firmware change.

## Обязательный следующий hardware regression

Обе платы должны быть прошиты из одной актуальной точки `cmp-protocol-v1`.

Проверить минимум:

1. обычный `JOB → JOB_ACK ACCEPTED`;
2. lost-ACK/pending-delivery cancel;
3. accepted no-run cancel;
4. повторную отмену уже очищенного job;
5. reboot ESP32 с persisted no-run uncertainty;
6. физический `D → * → # → D` → `ALL_CLEAR`;
7. блокировку emergency clear во время active/paused winding;
8. отсутствие любого automatic wire writeoff во всех recovery сценариях.

## Статус release baseline

Checkpoint `38` остается исторически верным для состояния **до** этого production firmware hardening.

После commit `1c66938c...` production code изменён, поэтому для текущей ветки нельзя утверждать, что старый hardware-accepted baseline автоматически покрывает новый cancel/recovery behavior.

Новый код уже прошел host/build verification, но hardware acceptance именно recovery-сценариев должен быть выполнен после прошивки обеих плат.

## Документация обновлена

В рамках этого checkpoint синхронизированы:

```text
docs/03_ARDUINO_CORE.md
docs/04_ESP32_CORE.md
docs/05_CMP_APPLICATION_PROTOCOL.md
docs/10_DIAGNOSTICS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
```

Этот файл является первым handoff checkpoint после `38` для всех вопросов JOB delivery/cancel/recovery.
