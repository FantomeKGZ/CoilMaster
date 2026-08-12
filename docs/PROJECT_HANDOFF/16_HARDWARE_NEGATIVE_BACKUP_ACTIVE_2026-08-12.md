# Hardware negative test — backup during active winding — 2026-08-12

Ветка: `cmp-protocol-v1`

## Подтверждено на реальном стенде

После fault-hardening commits пользователь прошил текущий ESP32 firmware и выполнил проверку backup guard во время активной намотки.

Проверенный сценарий:

```text
RUN_STARTED / активная намотка
→ GET /api/backup/manifest
→ backup/export заблокирован
→ deep snapshot stability audit не запускается
```

Пользователь подтвердил, что ожидаемые значения/признаки блокировки были найдены и поведение соответствует требуемому.

## Смысл проверки

Backup/deep-integrity не должен выполнять тяжёлое чтение microSD одновременно с активной намоткой или неопределённым recovery state.

Runtime guard теперь fail-closed считает `Busy` для:

```text
runActive
autonomousRunActive
jobAwaitingAck
jobCancelAwaitingAck
Accepted + completedRuns == 0
MANUAL_REVIEW_REQUIRED
```

Persisted fallback также блокирует export для незавершённых/неопределённых состояний:

```text
Created
Delivering
WaitingPhysicalStart
Running
Fault
```

## Статус

Сценарий `backup request during active winding` теперь считается **hardware validated** и удаляется из списка неподтверждённых production-hardening рисков.

Safety invariants не изменены:

- physical START только физический;
- ESP32/Web не управляют SSR;
- auto-resume отсутствует;
- `RUN_COMPLETED` не списывает провод автоматически;
- writeoff остаётся ручным exact-run/exact-spool.
