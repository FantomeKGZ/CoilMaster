# Hardware production E2E checkpoint — 2026-08-12

Ветка: `cmp-protocol-v1`

## Подтверждено на реальном стенде

Пользователь провёл первую реальную linked winding через основной production workflow CoilMaster.

Фактически подтверждён путь:

```text
client / motor / OPEN repair
→ linked winding
→ exact spool selection
→ UART delivery
→ physical START на Arduino
→ реальная намотка
→ RUN_STARTED / RUN_COMPLETED
→ manual exact-run wire writeoff
→ данные списания отображаются в интерфейсе
→ costing / finalization path
→ CLOSED / итоговые данные найдены
→ backup/manifest данные читаются
```

Отдельно подтверждено пользователем:

- первая реальная намотка выполнена успешно;
- ручное списание провода выполняется;
- данные списания отображаются;
- итоговые данные после завершения находятся корректно;
- ранее уже подтверждены Arduino local standalone flow, ESP32 clean build и `snapshot_stable=true` для deep backup manifest.

## Статус production E2E

Основной linked production E2E больше не считать неподтверждённым внешним риском.

Теперь hardware-validated:

```text
repair → exact spool → winding → exact-run writeoff → final data / CLOSED → backup visibility
```

Safety invariants при этом не изменялись:

- physical START остаётся только физическим;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` сам по себе не списывает провод;
- writeoff остаётся ручным и связан с exact `spool_id + source_session_id + source_run_id`.

## Что осталось до production-hardening completion

Основная функциональность и happy-path hardware E2E подтверждены. Следующий приоритет — fault/negative scenarios, а не добавление новой основной логики:

- reboot/manual-review во время незавершённого задания;
- microSD loss / unavailable storage;
- corrupted persisted data;
- UART timeout / reject / duplicate event;
- wrong spool / session / run;
- duplicate writeoff;
- close без required writeoff coverage;
- backup request во время active winding;
- benchmark на более наполненном реальном dataset перед любым Stage 1 performance refactor.

## Текущая оценка готовности

После успешного hardware production E2E:

```text
Functional completeness: ~99%
Repository/firmware readiness: ~98%
Operational readiness: ~96–97%
Overall project readiness: ~98%
```

GitHub CI отдельно остаётся неподтверждённым, если нет фактического CI result. Локальный ESP32 build ранее подтверждён пользователем как SUCCESS.
