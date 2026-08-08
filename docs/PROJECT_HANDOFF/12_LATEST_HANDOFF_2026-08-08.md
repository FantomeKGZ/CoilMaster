# CoilMaster — полный handoff на 2026-08-08

Ветка: `cmp-protocol-v1`  
Репозиторий: `FantomeKGZ/CoilMaster`

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1`; для нового файла сначала проверять отсутствие пути.

## Production flow

Уже собран и не должен проектироваться заново:

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART delivery → physical START
→ RUN_STARTED/RUN_COMPLETED → manual wire writeoff
→ source_session_id + source_run_id → materials/pricing
→ finalization preflight → CLOSED → archive/report → read-only backup
```

Базовые machine-safety invariants и fail-closed storage semantics сохранять без ослабления. Hardware E2E считается выполненным только после реального стенда и подтверждения пользователя.

## Уже завершённые repository blocks

Не повторять:

- persistent allocator, snapshot/state/recovery;
- authoritative repair/motor/coil_program linkage;
- exact spool selection;
- winding journal/history + transition audit;
- manual wire writeoff с session/run provenance;
- warehouse/material recovery и costing;
- repair finalization/CLOSED;
- reports/archive;
- read-only whitelist backup/export;
- deep backup persistence integrity;
- winding backup cleanup на `WindingJournalQuery::validateAll()`;
- backup/run-level HTTP semantics audit.

Deep backup при safe machine state проверяет allocator, conductor settings, workshop/pricing, materials, winding journal + transitions, warehouse persistence/movements и содержимое session snapshot/state/spool-selection. При active winding тяжёлый integrity scan не запускается.

## Winding cleanup — перепроверен по актуальному коду

`CM_WindingPersistenceIntegrityAudit` уже использует authoritative:

```text
WindingJournalQuery::validateAll(recordCount)
WindingJournalTransitionAudit::validate()
```

Реализация `validateAll()` находится в отдельном translation unit:

```text
firmware/esp32/src/CM_WindingJournalQueryValidation.cpp
```

Root `platformio.ini` для `env:esp32` включает `firmware/esp32/src/*.cpp`, поэтому этот implementation unit входит в ESP32 source filter. `CM_WindingSessionPersistenceIntegrityAudit` остаётся отдельным authoritative deep audit для snapshot/state/spool-selection и cross-file identity; не дублировать его.

## Stage 0 performance observability

Решение остаётся: не мигрировать в БД и не вводить rotation threshold до реальных измерений.

Manifest теперь возвращает без дополнительного telemetry full scan двадцать runtime metrics:

```text
snapshot_stability_duration_ms
winding_allocator_last_id
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
workshop_client_record_count
workshop_motor_record_count
workshop_repair_record_count
repair_status_record_count
repair_pricing_record_count
winding_journal_record_count
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
winding_snapshot_total_bytes
winding_state_total_bytes
winding_spool_selection_total_bytes
warehouse_spool_record_count
warehouse_price_record_count
warehouse_movement_record_count
```

### Persistent allocator high-water

`PersistentIdIntegrityAuditMetrics` возвращает:

```text
lastAllocatedId
```

Старый `PersistentIdIntegrityAudit::check(storage)` сохранён и делегирует metrics overload. `winding_allocator_last_id` берётся из уже выполняемого чтения и проверки `id-state.txt`, где `last_job_id == last_session_id`. При отсутствии pristine `/data/winding-jobs` successful audit даёт `0`. При invalid main/backup state метрика не публикуется. Дополнительного чтения allocator state ради telemetry нет.

### Winding journal

`winding_journal_record_count` считается в существующем authoritative EOF-pass `WindingJournalQuery::validateAll()` и публикуется только после успешной winding schema + transition validation.

### Winding session persistence

`WindingSessionPersistenceAuditMetrics` возвращает:

```text
snapshotFileCount
stateFileCount
spoolSelectionFileCount
byteTotalsAvailable
snapshotTotalBytes
stateTotalBytes
spoolSelectionTotalBytes
```

Старый `WindingSessionPersistenceIntegrityAudit::check(storage)` сохранён и делегирует metrics overload. Counts и byte totals считаются в существующих deep parser/cross-identity directory passes по session files; дополнительного directory/full-file pass ради telemetry нет.

Byte totals берутся через уже открытый `File entry` до штатного parser/load. Они являются telemetry-only: если суммарный размер не помещается в 32-bit, `byteTotalsAvailable=false`, три total-byte поля manifest становятся `null`, но сам integrity audit продолжает прежнюю fail-closed validation. Counts/bytes публикуются только после полного successful session persistence audit.

### Materials

`MaterialPersistenceAuditMetrics` возвращает:

```text
materialRecordCount
usageRecordCount
adjustmentRecordCount
```

Старый `MaterialPersistenceIntegrityAudit::check(storage)` сохранён и делегирует metrics overload. Counts собираются в уже существующих scans `materials.ndjson`, `usage.ndjson`, `adjustments.ndjson`; дополнительного чтения файлов ради telemetry нет. Счётчики публикуются только после успешного текущего material persistence audit. Если конкретный material файл отсутствует, successful audit даёт `0`, а наличие файла отдельно видно в manifest `items[].exists`.

### Business/workshop/pricing

`BackupBusinessDataAuditMetrics` возвращает:

```text
clientRecordCount
motorRecordCount
repairRecordCount
repairStatusRecordCount
pricingRecordCount
```

Старый `BackupBusinessDataIntegrityAudit::check(storage)` сохранён и делегирует metrics overload. Client/motor/repair counts собираются в уже существующих uniqueness scans, repair-status/pricing counts — в существующих parser/reference passes. Partial counts при failed business audit наружу не публикуются; дополнительного full-file чтения ради telemetry нет.

### Warehouse persistence

`WarehousePersistenceAuditMetrics` возвращает:

```text
spoolRecordCount
priceRecordCount
```

Старый `WarehousePersistenceIntegrityAudit::check(storage)` сохранён и делегирует metrics overload. `spoolRecordCount` и `priceRecordCount` считаются внутри уже существующих spool/price validation loops. Metrics копируются наружу только после успешного spool + price + movement-reference audit, поэтому partial counts при failure не публикуются. Отсутствующий spool/price файл при successful audit даёт `0`; отдельного full-file scan ради telemetry нет.

### Warehouse movements

`warehouse_movement_record_count` считается внутри существующего `WarehouseMovementIntegrityAudit::check()` pass. Считаются непустые persisted transaction rows, включая `PENDING` и завершающие `CONFIRMED|ABORTED`. Старый `check(storage)` сохранён, добавлен совместимый count overload.

## Последние code commits Stage 0

```text
cf7df132d190bd359a4f4b85b2553f6dcdba5dd4  Expose business data audit counts
33746bf301ee8a31417361ce6fd8f7a2ce1635f7  Count business backup audit records
1871d140e1e493b6e64ada3502eaa5fbcb75f0f6  Expose business audit counts in backup manifest
fdb2428895004b7f248504bab8ef85334651535a  Expose warehouse persistence audit counts
490bee61c81f6425bde5623818fdf80abaa089dc  Count warehouse persistence audit records
34645b743e5638d379287947a8a62ec61ea4ee90  Expose warehouse persistence counts in backup manifest
4a30e4ca08e1d2e010dded1ab3e93073f9ecaeed  Expose persistent allocator audit metrics
b38bb3b5190bb99d261f5552cecedfea4048289b  Return validated allocator high-water mark
52fae7716034ccacebc41f1f11715f5eebf193c2  Expose allocator high-water mark in backup manifest
1470b866c0b91aee4bd8dff1eddc6c26926be578  Expose winding session byte totals
cacdffa9ec822ad1425d6a4de34c10f836fbbab0  Measure winding session persistence bytes
a0c83b08f64c05f0232d287146850f9e9fd37ce5  Expose winding session byte totals in backup manifest
```

Документационные commits последнего batch:

```text
19f026e778675a5423fa446e6595bc1115547f85  Document allocator and session byte observability
5d39cef6b454c21aac37b2a80c360dcd81e6d615  Record allocator and session byte metrics
ba8f7c89d5f23f0549608b7603f6503d4fd6d514  Advance active work observability metrics
599610f1786f7e68cd45c6da40ee7acf9b8093fd  Index allocator and session byte metrics
```

Эти commits не считать GREEN без фактического Actions result.

## Metrics integration review

На уровне static repository review подтверждено:

- compatibility `check(storage)` сохранён у добавленных metrics overloads;
- public metrics headers самостоятельно включают требуемые типы (`FS.h`, `stdint.h` там, где добавлены uint32 metrics);
- allocator high-water берётся из уже выполняемого authoritative allocator read;
- record counts считаются внутри уже существующих validation loops/passes;
- session byte totals считаются через уже открытые directory entries без дополнительного scan;
- telemetry overflow session byte totals не превращается в persistence corruption;
- partial business/material/session/warehouse/allocator metrics не публикуются после failed domain audit;
- `BackupActivityGuard::Safe` gating не изменён;
- winding `validateAll()` и session deep parser/cross-identity audit не заменены и не продублированы telemetry-логикой;
- safety boundary физического START/SSR/writeoff не затронута.

Это static repository review, а не доказательство успешного ESP32 build.

## Найденные composition/performance hotspots

Полный `MaterialPersistenceIntegrityAudit::check()` после собственных material scans транзитивно вызывает:

```text
WorkshopPersistenceIntegrityAudit::check()
RepairPricingIntegrityAudit::check()
```

`WorkshopPersistenceIntegrityAudit` затем снова проверяет warehouse, allocator, winding session persistence, winding journal schema и transitions. Backup orchestration позже проверяет эти domains ещё раз отдельными audit’ами.

Кроме того, `BackupBusinessDataIntegrityAudit` намеренно использует повторные uniqueness/reference scans, а warehouse movement-reference validation повторно ищет spool/repair references. Это не correctness bug; новые population/count/size/high-water metrics позволяют измерить влияние по реальным размерам.

После benchmark безопасный Stage 1 вариант — разделить local material-file validation и cross-domain dependency validation так, чтобы backup orchestration выполнял каждый authoritative domain audit один раз, сохранив старые public contracts для других callers. Bounded in-request indexes для business/warehouse reference lookups рассматривать только если измерения покажут hotspot, с явным RAM limit и fail-closed поведением.

Не начинать эти refactors только ради эстетики до измерений, если не обнаружена отдельная correctness проблема.

## Что измерить на hardware/E2E стенде

Сохранить один и тот же manifest/замер вместе с:

```text
workshop-clients.size_bytes
workshop_client_record_count
workshop-motors.size_bytes
workshop_motor_record_count
workshop-repairs.size_bytes
workshop_repair_record_count
repair-status.size_bytes
repair_status_record_count
repair-pricing.size_bytes
repair_pricing_record_count
materials.size_bytes
material_catalog_record_count
material-usage.size_bytes
material_usage_record_count
material-adjustments.size_bytes
material_adjustment_record_count
winding-events.size_bytes
winding_journal_record_count
winding_allocator_last_id
winding_snapshot_file_count
winding_snapshot_total_bytes
winding_state_file_count
winding_state_total_bytes
winding_spool_selection_file_count
winding_spool_selection_total_bytes
warehouse-spools.size_bytes
warehouse_spool_record_count
warehouse-price.size_bytes
warehouse_price_record_count
warehouse-movements.size_bytes
warehouse_movement_record_count
snapshot_stability_duration_ms
```

По этим данным выбирать первый реальный hotspot. До этого не вводить persistent optimistic cache, arbitrary rotation threshold, Stage 1 duplicate-scan refactor или database migration.

## Точная следующая repo-only точка

Если hardware пока недоступен, продолжать только low-cost same-pass observability существующих authoritative validators, без второго full scan ради telemetry, либо исправлять отдельную доказанную correctness/compile проблему.

Если новый Actions run доступен — проверять фактический `build-esp32` head и исправлять первую реальную compile/link error. Последняя ранее подтверждённая compile problem была missing namespace closing brace в `CM_MaterialLedger.cpp`, исправленная commit:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8
```

Hardware E2E ESP32 + Arduino остаётся обязательным внешним этапом и repository review его не заменяет.

## Порядок чтения следующему чату

```text
1. docs/PROJECT_HANDOFF/00_READ_FIRST.md
2. docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md
3. docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
4. docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
5. актуальные исходники конкретного изменения
6. docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
7. docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md
8. docs/PROJECT_HANDOFF/09_KEY_FILES_INDEX.md
9. docs/PROJECT_HANDOFF/08_WORK_RULES_AND_VERIFICATION.md
```
