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

Manifest теперь возвращает без дополнительного telemetry full scan шестнадцать runtime metrics:

```text
snapshot_stability_duration_ms
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
warehouse_spool_record_count
warehouse_price_record_count
warehouse_movement_record_count
```

### Winding journal

`winding_journal_record_count` считается в существующем authoritative EOF-pass `WindingJournalQuery::validateAll()` и публикуется только после успешной winding schema + transition validation.

### Winding session persistence

`WindingSessionPersistenceAuditMetrics` возвращает:

```text
snapshotFileCount
stateFileCount
spoolSelectionFileCount
```

Старый `WindingSessionPersistenceIntegrityAudit::check(storage)` сохранён и делегирует metrics overload. Counts считаются в существующих deep parser/cross-identity проходах по session files и публикуются только после полного успешного session persistence audit. Отдельного directory/full-file pass ради telemetry нет.

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

Кодовые commits текущего расширения Stage 0:

```text
63614fe363adaf912fdf35775ecff6befad34ed6  Expose warehouse movement audit count
fe024d6908e4488e114b633c97d06848d2d9bc38  Count warehouse movement audit records
a78cf149dd5d1f588988ddda3e2d046459fd36b5  Expose warehouse movement audit count
ac031d8cc14a74786e10c8adb782776b0d16e97f  Expose material persistence audit counts
6cf4ad7da157c8e65f131b9a851c4243c0914e31  Count material persistence audit records
38befe338cfc57879d2ad09fc6be54d54c190441  Expose material audit counts in backup manifest
9c33178d8b580460e1d34962322fe81b9771dccc  Expose winding session persistence counts
afd2c9e3df2e63b59553e4f10e12eb4d2199e46d  Count winding session persistence files
abc4b02ef284ed86fdfc3e31149ccf8adf9d5e8b  Expose winding session file counts in backup manifest
cf7df132d190bd359a4f4b85b2553f6dcdba5dd4  Expose business data audit counts
33746bf301ee8a31417361ce6fd8f7a2ce1635f7  Count business backup audit records
1871d140e1e493b6e64ada3502eaa5fbcb75f0f6  Expose business audit counts in backup manifest
fdb2428895004b7f248504bab8ef85334651535a  Expose warehouse persistence audit counts
490bee61c81f6425bde5623818fdf80abaa089dc  Count warehouse persistence audit records
34645b743e5638d379287947a8a62ec61ea4ee90  Expose warehouse persistence counts in backup manifest
```

Документация последних observability шагов:

```text
0f10ed32d110c28b21af7c46c6be20a084c6ba2b  Document material backup observability
899508534fc909d9baacada62bcda8629ddf0b4a  Advance active work to material observability
1a78e23bdfc3addb7826845fc8273b33d96f5e67  Record material backup observability in current state
9f32a1c953017447a71d907535ac2cf0398a5098  Refresh backup and winding key file index
5a7070d805eb902e11046b7dfe94af169d549e99  Record material metrics integration review
7cb635d4088d38cef58dcb39bc170bf7f55dd0c7  Document winding session backup observability
702edf98c9b993332fdaf17aa2359c1128391f27  Document business backup observability
b13bcb6d19fe9aaac13d4a01a273e9823dd4fb50  Complete business observability handoff
d43e2361fadffa0191f31a975cc639f17e90e0c3  Document warehouse persistence backup observability
7e7521122bdde5060fef2e011f6b5a20f80079ea  Record warehouse backup observability in current state
36741e8d129effb8d5b70c7a678e4a4e81528502  Advance active work to warehouse observability benchmark
3a60aed16e77a089f38f79514b7e48413af54dcc  Index warehouse backup audit metrics
```

Эти commits не считать GREEN без фактического Actions result.

## Metrics integration review

Повторно просмотрены актуальные business/material/winding-session/warehouse audit contracts и `CM_BackupExportWeb.cpp`.

На уровне static repository review подтверждено:

- compatibility `check(storage)` сохранён у добавленных metrics overloads;
- public metrics headers самостоятельно включают `FS.h` и `stdint.h`;
- counters считаются внутри уже существующих validation loops/passes;
- partial business/material/session/warehouse persistence metrics не публикуются после failed domain audit;
- `BackupActivityGuard::Safe` gating не изменён;
- winding `validateAll()` и session deep parser/cross-identity audit не заменены и не продублированы telemetry-логикой;
- warehouse spool/price counters не создают дополнительный full-file pass;
- safety boundary физического START/SSR/writeoff не затронута.

Это static repository review, а не доказательство успешного ESP32 build.

## Найденные composition/performance hotspots

Полный `MaterialPersistenceIntegrityAudit::check()` после собственных material scans транзитивно вызывает:

```text
WorkshopPersistenceIntegrityAudit::check()
RepairPricingIntegrityAudit::check()
```

`WorkshopPersistenceIntegrityAudit` затем снова проверяет warehouse, allocator, winding session persistence, winding journal schema и transitions. Backup orchestration позже проверяет эти domains ещё раз отдельными audit’ами.

Кроме того, `BackupBusinessDataIntegrityAudit` намеренно использует повторные uniqueness/reference scans, а warehouse movement-reference validation повторно ищет spool/repair references. Это не correctness bug; новые population counts позволяют измерить влияние по реальным размерам.

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
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
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
