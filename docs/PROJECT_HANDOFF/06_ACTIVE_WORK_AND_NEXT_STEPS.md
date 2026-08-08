# Где остановились и что делать дальше

Дата обновления: 2026-08-08 22:28 +06  
Ветка: `cmp-protocol-v1`

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

Самый свежий pause-snapshot до возобновления:

```text
docs/PROJECT_HANDOFF/13_PAUSE_HANDOFF_2026-08-08_2222.md
```

## Текущая оценка готовности

Ориентировочно:

- реализованная функциональность production flow: **около 96%**;
- repository/firmware readiness: **около 93%**;
- общая эксплуатационная готовность: **около 90%**.

Эксплуатационная оценка остаётся ниже repository readiness: для текущего HEAD нет подтверждённого GREEN Actions result, полный ESP32 + Arduino hardware E2E ещё не выполнен, Stage 0 benchmark на реальном dataset ещё не снят.

## Текущий code checkpoint

```text
4c7051603fc8753ed04caed025b27dc61f628136  Restore warehouse writeoff provenance lookups
```

Восстановлены определения объявленных методов:

```text
WarehouseStore::confirmedWriteOffForSourceSession(...)
WarehouseStore::confirmedWriteOffForSourceRun(...)
```

Файл:

```text
firmware/esp32/src/CM_WarehouseWriteOffLookup.cpp
```

Перед read-only lookup выполняется authoritative `WarehouseMovementIntegrityAudit::check()`. Поэтому malformed movement history, broken PENDING → terminal transaction и ambiguous/duplicate provenance fail closed. Legacy session-only CONFIRMED запись может быть найдена только session-level lookup; она **не** считается exact-run совпадением.

`platformio.ini` включает `firmware/esp32/src/*.cpp`, поэтому новый translation unit входит в ESP32 build source filter. Статически сигнатуры совпадают с `CM_WarehouseStore.h` и с `CM_WarehouseMovementIntegrityAudit.h`.

Фактический ESP32 linker/build для этого HEAD всё ещё **CI NOT CONFIRMED**: GitHub connector возвращает пустой combined status и не показывает push workflow run.

## Production flow

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual exact-run wire writeoff
→ materials/pricing → finalization preflight
→ CLOSED → archive/report → read-only backup
```

## Safety invariants — не менять

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- automatic resume после reboot отсутствует;
- `RUN_COMPLETED` сам по себе не выполняет wire writeoff;
- wire writeoff остаётся ручным;
- новый writeoff требует exact `spool_id + source_session_id + source_run_id`;
- corrupted persistence/storage loss блокирует опасную операцию fail-closed.

## Exact-run writeoff provenance — закрытый блок

Storage boundary:

```text
c7335631c660a7b5ee71da880a1f77e4e5faa83f  Require exact run provenance for new wire writeoffs
```

HTTP boundary:

```text
7b92010294342d2a9cc9a153f306673f5c66ffb9  Require run provenance in wire writeoff API
```

UI boundary:

```text
ef0e64838ebb3f0519f6bfe756ade599a07450b9  Require exact run provenance in writeoff UI
```

Lookup/link boundary:

```text
4c7051603fc8753ed04caed025b27dc61f628136  Restore warehouse writeoff provenance lookups
```

Новые writeoff нельзя создавать без exact session/run. Shared mobile/desktop helper только предлагает immutable spool и attaches exact completed run; фактический расход/вес остаётся ручным подтверждением оператора.

Legacy session-only records остаются grandfathered/read-only compatibility старых данных; не выполнять их автоматическую миграцию в guessed run IDs.

## Production bootstrap — актуальное состояние

Реально зарегистрированы:

```text
workshop clients/motors/repairs
motor similarity
warehouse summary/spools/price
warehouse writeoff GET/POST
materials CRUD/usage/adjustments
repair costing/pricing history
conductor calculator/settings
backup manifest/file/session export
winding history
```

`/api/winding-history` регистрируется только через `StaticSiteServer::begin()`. Не добавлять второй handler в `WarehouseWeb::begin()`.

## Persistence / backup integrity

Authoritative winding checks:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

Production conductor settings source:

```text
/data/settings/conductor.json
```

Deep backup safe state охватывает allocator, conductor settings, workshop/pricing, materials, winding journal/transitions, warehouse movements, snapshot/state/spool-selection contents и cross-file identity.

Stage 0 observability уже содержит **29 metrics**. Не добавлять новые telemetry scans до benchmark.

## Следующее действие

Repository-only compile/link gap, ради которого была сделана пауза, закрыт кодом. Не продолжать бесконечный parser-hardening и не начинать Stage 1 performance refactor без измерений.

Обязательный следующий внешний этап — реальный ESP32 + Arduino hardware E2E:

```text
linked repair
→ exact spool
→ JOB_ACK
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ manual exact-run wire writeoff
→ costing
→ finalization preflight
→ CLOSED
→ stable backup
```

Fault cases:

- reboot/manual-review;
- microSD loss;
- corrupted persistence;
- UART timeout/reject/duplicate events;
- duplicate writeoff;
- missing/wrong session/run/spool;
- close without wire coverage;
- backup during active winding.

Одновременно сохранить один `/api/backup/manifest` с:

```text
items[].size_bytes
snapshot_stability_duration_ms
9 per-domain *_duration_ms
winding_allocator_last_id
material/business/winding/warehouse record counts
winding snapshot/state/spool-selection counts + total bytes
```

После benchmark выбрать самый дорогой `*_duration_ms` и только тогда решать, нужен ли bounded index, decomposition repeated scans или rotation. Database migration, persistent optimistic cache и arbitrary rotation threshold до измерений не вводить.
