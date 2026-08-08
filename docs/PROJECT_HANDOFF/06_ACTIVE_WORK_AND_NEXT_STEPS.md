# Где остановились и что делать дальше

Дата обновления: 2026-08-08 23:49 +06  
Ветка: `cmp-protocol-v1`

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

Самый свежий pause-snapshot до возобновления:

```text
docs/PROJECT_HANDOFF/13_PAUSE_HANDOFF_2026-08-08_2222.md
```

## Текущая оценка готовности

Ориентировочно:

- реализованная функциональность production flow: **около 96%**;
- repository/firmware readiness: **около 94%**;
- общая эксплуатационная готовность: **около 90%**.

Repository readiness немного повышена после закрытия конкретных compile/link и storage-boundary writeoff gaps. Эксплуатационная оценка остаётся ниже: для текущего HEAD нет подтверждённого GREEN Actions result, полный ESP32 + Arduino hardware E2E ещё не выполнен, Stage 0 benchmark на реальном dataset ещё не снят.

## Текущий code checkpoint

```text
eb95164c1ecbc025835df1c20308b0d53605e558  Reject unstable spool selection temp state
```

После возобновления закрыты следующие repo-level проблемы.

### 1. Warehouse writeoff provenance lookup definitions

```text
4c7051603fc8753ed04caed025b27dc61f628136  Restore warehouse writeoff provenance lookups
```

Файл:

```text
firmware/esp32/src/CM_WarehouseWriteOffLookup.cpp
```

Восстановлены определения:

```text
WarehouseStore::confirmedWriteOffForSourceSession(...)
WarehouseStore::confirmedWriteOffForSourceRun(...)
```

Lookup fail closed через authoritative `WarehouseMovementIntegrityAudit::check()`. Legacy session-only CONFIRMED запись не считается exact-run совпадением.

### 2. HTTP repair lifecycle failure classification

```text
7c210f013d6a3fbc1d0534b37d56c8fa808882ee  Classify repair lifecycle integrity failures
```

`POST /api/warehouse/write-offs` теперь различает:

```text
storage/warehouse unavailable -> 503 repair_lifecycle_unavailable
repair-status persistence/integrity failure при доступном storage -> 500 repair_lifecycle_integrity_failed
closed repair -> 409 repair_closed
```

Списание блокируется во всех failure cases; менялась только корректная HTTP/error classification.

### 3. Duplicate spool-selection symbol removed

Был подтверждён реальный linker-risk: tri-state

```text
JobSpoolSelectionStore::load(uint32_t, JobSpoolSelection&, bool&) const
```

одновременно определялся в:

```text
CM_JobSpoolSelectionStore.cpp
CM_JobSpoolSelectionLookup.cpp
```

Оба `.cpp` входят в `firmware/esp32/src/*.cpp`, поэтому duplicate definition был compile/link correctness problem.

Исправлено:

```text
c243d37e8b2ec8c65ac6872fbd926d3bd52511fe  Remove duplicate spool selection lookup definition
```

Canonical tri-state implementation теперь находится только в:

```text
firmware/esp32/src/CM_JobSpoolSelectionLookup.cpp
```

`CM_JobSpoolSelectionStore.cpp` сохраняет обычный wrapper `load(sessionId, selection)`.

### 4. Read-only immutable spool-selection lookup

Добавлен exact-file read-only lookup без `begin()`, directory scan или temp recovery:

```text
aea99008f857c679b332fa6899bb774afb0ec61c  Expose read-only spool selection lookup
b3a46d3784a134fc475ca745b214d8dc376fb34b  Add read-only spool selection lookup
eb95164c1ecbc025835df1c20308b0d53605e558  Reject unstable spool selection temp state
```

API:

```text
JobSpoolSelectionStore::loadReadOnly(storage, sessionId, selection, found)
```

Он читает только canonical:

```text
/data/winding-jobs/spool-selection/session-<session_id>.json
```

и использует существующий strict parser. Если для exact session присутствует `session-<id>.json.tmp`, lookup fail closed как unstable/ambiguous selection state. Это позволяет core safety checks читать immutable selection без повторного полного directory audit и без автоматического runtime recovery.

### 5. Exact spool selection теперь enforced на storage boundary

```text
979e81acd4c67da66c1b74b9995c97bf648986d3  Enforce immutable spool selection in storage writeoff
```

`WarehouseStore::confirmSpoolWriteOff()` теперь до `RUN_COMPLETED` proof и до создания `PENDING` сам требует:

```text
source_session_id -> immutable spool selection exists
selection.repair_id == operation.repair_id
selection.spool_id  == operation.spool_id
```

Поэтому internal/direct caller больше не может обойти HTTP и списать другой spool для завершённого run. Web остаётся дополнительным preflight layer, но safety invariant теперь защищён на storage boundary.

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
- storage boundary дополнительно требует immutable session → repair/spool match;
- exact-session `.json.tmp` блокирует writeoff fail closed;
- corrupted persistence/storage loss блокирует опасную операцию fail-closed.

## Exact-run writeoff provenance — закрытый блок

Ключевые commits:

```text
c7335631c660a7b5ee71da880a1f77e4e5faa83f  Require exact run provenance for new wire writeoffs
7b92010294342d2a9cc9a153f306673f5c66ffb9  Require run provenance in wire writeoff API
ef0e64838ebb3f0519f6bfe756ade599a07450b9  Require exact run provenance in writeoff UI
4c7051603fc8753ed04caed025b27dc61f628136  Restore warehouse writeoff provenance lookups
979e81acd4c67da66c1b74b9995c97bf648986d3  Enforce immutable spool selection in storage writeoff
eb95164c1ecbc025835df1c20308b0d53605e558  Reject unstable spool selection temp state
```

Legacy session-only records остаются grandfathered/read-only compatibility старых данных; не выполнять автоматическую миграцию в guessed run IDs.

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

## CI / compile verification

Последний code-only HEAD перед этим handoff update:

```text
eb95164c1ecbc025835df1c20308b0d53605e558
```

Branch compare подтверждал `cmp-protocol-v1` identical этому SHA.

GitHub connector возвращает пустой combined status. Публичная Actions страница также не была надёжно получена через доступный web fetch. Поэтому состояние остаётся:

```text
CI NOT CONFIRMED
```

Не называть текущий ESP32 build GREEN без фактического Actions result.

## Следующее действие

После закрытия найденных compile/link и storage-boundary gaps не продолжать бесконечный parser-hardening и не начинать Stage 1 performance refactor без измерений.

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

Особенно проверить negative cases:

```text
completed source_session_id/source_run_id
+ другой spool_id, не совпадающий с immutable selection
=> writeoff MUST be rejected before PENDING/mutation

exact session spool-selection .json.tmp present
=> writeoff MUST be rejected before PENDING/mutation
```

Fault cases:

- reboot/manual-review;
- microSD loss;
- corrupted persistence;
- UART timeout/reject/duplicate events;
- duplicate writeoff;
- missing/wrong session/run/spool;
- wrong spool against immutable selection;
- unstable exact-session spool-selection temp;
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
