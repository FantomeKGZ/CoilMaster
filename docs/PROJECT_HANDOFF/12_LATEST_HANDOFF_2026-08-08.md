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

Manifest теперь возвращает без дополнительного full scan:

```text
snapshot_stability_duration_ms
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
winding_journal_record_count
warehouse_movement_record_count
```

### Winding

`winding_journal_record_count` считается в существующем authoritative EOF-pass `WindingJournalQuery::validateAll()` и публикуется только после успешной winding schema + transition validation.

### Warehouse movements

`warehouse_movement_record_count` считается внутри существующего `WarehouseMovementIntegrityAudit::check()` pass. Считаются непустые persisted transaction rows, включая `PENDING` и завершающие `CONFIRMED|ABORTED`. Старый `check(storage)` сохранён, добавлен совместимый count overload.

### Materials

Добавлен `MaterialPersistenceAuditMetrics`:

```text
materialRecordCount
usageRecordCount
adjustmentRecordCount
```

Старый `MaterialPersistenceIntegrityAudit::check(storage)` сохранён и делегирует metrics overload. Counts собираются в уже существующих scans `materials.ndjson`, `usage.ndjson`, `adjustments.ndjson`; дополнительного чтения файлов ради telemetry нет. Счётчики публикуются только после успешного текущего material persistence audit. Если конкретный material файл отсутствует, successful audit даёт `0`, а наличие файла отдельно видно в manifest `items[].exists`.

Кодовые commits текущего расширения Stage 0:

```text
63614fe363adaf912fdf35775ecff6befad34ed6  Expose warehouse movement audit count
fe024d6908e4488e114b633c97d06848d2d9bc38  Count warehouse movement audit records
a78cf149dd5d1f588988ddda3e2d046459fd36b5  Expose warehouse movement audit count
ac031d8cc14a74786e10c8adb782776b0d16e97f  Expose material persistence audit counts
6cf4ad7da157c8e65f131b9a851c4243c0914e31  Count material persistence audit records
38befe338cfc57879d2ad09fc6be54d54c190441  Expose material audit counts in backup manifest
```

Документация:

```text
0f10ed32d110c28b21af7c46c6be20a084c6ba2b  Document material backup observability
899508534fc909d9baacada62bcda8629ddf0b4a  Advance active work to material observability
1a78e23bdfc3addb7826845fc8273b33d96f5e67  Record material backup observability in current state
9f32a1c953017447a71d907535ac2cf0398a5098  Refresh backup and winding key file index
5a7070d805eb902e11046b7dfe94af169d549e99  Record material metrics integration review
```

Эти commits не считать GREEN без фактического Actions result.

## Material metrics integration review

Повторно просмотрены актуальные `CM_MaterialPersistenceIntegrityAudit.h/.cpp`, `CM_BackupExportWeb.cpp`, `CM_WorkshopPersistenceIntegrityAudit.cpp` и `CM_RepairPricingIntegrityAudit.cpp`.

На уровне static repository review подтверждено:

- compatibility `check(storage)` сохранён;
- public metrics header самостоятельно включает `FS.h` и `stdint.h`;
- counters считаются внутри уже существующих validation loops;
- partial metrics не публикуются при failure полного material audit;
- `BackupActivityGuard::Safe` gating не изменён;
- winding/session deep audits не заменены и не продублированы material observability логикой.

Отдельной correctness-причины для дополнительного code refactor в этом review не найдено. Это не доказательство успешного ESP32 build.

## Найденный composition hotspot

Repository review подтвердил, что полный `MaterialPersistenceIntegrityAudit::check()` после собственных material scans транзитивно вызывает:

```text
WorkshopPersistenceIntegrityAudit::check()
RepairPricingIntegrityAudit::check()
```

`WorkshopPersistenceIntegrityAudit` затем снова проверяет warehouse, allocator, winding session persistence, winding journal schema и transitions. Backup orchestration позже проверяет эти domains ещё раз отдельными audit’ами.

Это доказанный duplicate-I/O path, но пока не повод преждевременно менять storage format. После benchmark безопасный Stage 1 вариант — разделить local material-file validation и cross-domain dependency validation так, чтобы backup orchestration выполнял каждый authoritative domain audit один раз, сохранив старые public contracts для других callers.

Не начинать этот refactor только ради эстетики до измерений, если не обнаружена отдельная correctness проблема.

## Что измерить на hardware/E2E стенде

Сохранить один и тот же manifest/замер вместе с:

```text
materials.size_bytes
material_catalog_record_count
material-usage.size_bytes
material_usage_record_count
material-adjustments.size_bytes
material_adjustment_record_count
winding-events.size_bytes
winding_journal_record_count
warehouse-movements.size_bytes
warehouse_movement_record_count
snapshot_stability_duration_ms
```

По этим данным выбирать первый реальный hotspot. До этого не вводить persistent optimistic cache, arbitrary rotation threshold или database migration.

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
