# CoilMaster — полный handoff на 2026-08-08

Ветка: `cmp-protocol-v1`  
Репозиторий: `FantomeKGZ/CoilMaster`

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать его текущий SHA.

## Production flow — уже собран

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART delivery → physical START
→ RUN_STARTED/RUN_COMPLETED → manual wire writeoff
→ source_session_id + source_run_id → materials/pricing
→ finalization preflight → CLOSED → archive/report → read-only backup
```

Не проектировать этот flow заново.

Safety invariants:

- никакого automatic physical START;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
- `RUN_COMPLETED` сам по себе не списывает провод;
- wire writeoff остаётся ручным и связан с exact `spool_id`, `source_session_id + source_run_id`.

Hardware E2E считается выполненным только после реального ESP32 + Arduino стенда.

## Уже закрытые repository blocks

Не повторять:

- persistent allocator, snapshot/state/recovery;
- authoritative repair/motor/coil_program linkage;
- exact spool selection;
- winding journal/history + transition audit;
- manual wire writeoff с run-level provenance;
- warehouse/material recovery и costing;
- finalization/CLOSED;
- reports/archive;
- read-only whitelist backup/export;
- deep backup persistence integrity;
- winding backup cleanup на `WindingJournalQuery::validateAll()`;
- backup/run-level HTTP semantics audit;
- Stage 0 backup observability до 29 metrics;
- flat persisted JSON syntax hardening для workshop/pricing/material/warehouse/settings.

Deep backup выполняется только при `BackupActivityGuard::Safe`. При active winding тяжёлый deep scan не запускается.

## Winding persistence cleanup

`CM_WindingPersistenceIntegrityAudit` уже использует:

```text
WindingJournalQuery::validateAll(recordCount)
WindingJournalTransitionAudit::validate()
```

Authoritative `validateAll()` реализован в:

```text
firmware/esp32/src/CM_WindingJournalQueryValidation.cpp
```

`CM_WindingSessionPersistenceIntegrityAudit` остаётся отдельным authoritative deep parser/cross-file identity audit для snapshot/state/spool-selection. Не дублировать его.

## Deep backup integrity coverage

Safe `snapshot_stable=true` требует read-only integrity всего static whitelist и adjuncts:

- persistent allocator main/optional backup, no `id-state.tmp`;
- conductor settings, no temp/bak recovery residue;
- workshop clients/motors/repairs + repair-status;
- pricing + repair references;
- materials/usage/adjustments + arithmetic/references/recovery markers;
- winding journal schema до EOF + transition semantics;
- warehouse spools/price/movements + references;
- canonical session directories;
- содержимое всех snapshot/state/spool-selection штатными parsers;
- cross-file `session_id/job_id/repair_id/motor_id/spool` identity.

## Stage 0 performance observability — 29 metrics

Решение: сначала измерять. До реального benchmark не вводить arbitrary rotation threshold, persistent optimistic cache, database migration или Stage 1 duplicate-scan refactor без отдельной correctness-причины.

Manifest сейчас возвращает 29 runtime metrics без дополнительного telemetry full scan.

### Total + per-domain durations

```text
snapshot_stability_duration_ms
persistent_id_audit_duration_ms
conductor_settings_audit_duration_ms
material_persistence_audit_duration_ms
business_data_audit_duration_ms
winding_persistence_audit_duration_ms
warehouse_persistence_audit_duration_ms
warehouse_movements_audit_duration_ms
winding_session_directory_scan_duration_ms
winding_session_persistence_audit_duration_ms
```

Per-domain timing оборачивает уже существующие audit calls через `millis()`. Дополнительного filesystem I/O нет.

Если deep audit не запускался, поля `null`. Если audit дошёл до domain, его duration публикуется даже при failure этого domain; последующие неисполненные domains остаются `null`.

Отдельно измеряются preliminary session-directory scan и authoritative deep session persistence audit. Пока не объединять их до benchmark.

### Population / high-water / bytes

```text
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

Semantics:

- allocator high-water берётся из уже выполняемого validated `id-state.txt` read;
- record counts считаются внутри уже существующих authoritative validation passes;
- session counts/bytes собираются внутри deep session directory passes;
- session aggregate byte overflow является telemetry-only: три byte totals становятся `null`, integrity result не меняется;
- compatibility `check(storage)` overloads сохранены;
- partial counts/high-water при failed domain audit наружу не публикуются.

## Flat persisted JSON correctness hardening

Repository review выявил correctness gap, независимый от benchmark: часть flat NDJSON readers проверяла строку только по внешним `{...}` и выбранным полям. Синтаксически повреждённая строка с читаемым ID могла пройти часть startup/deep checks или попасть в read-only JSON API.

Добавлен общий header-only validator:

```text
firmware/esp32/src/CM_FlatJsonObjectValidator.h
```

Он проверяет синтаксис уже прочитанного flat JSON object без внешней JSON dependency и без нового SD pass.

Hardened readers/audits:

```text
CM_BackupBusinessDataIntegrityAudit
CM_RepairRegistry
CM_RepairPricingIntegrityAudit
CM_MaterialPersistenceIntegrityAudit
CM_WarehousePersistenceIntegrityAudit
CM_WarehouseMovementIntegrityAudit
CM_ConductorSettingsIntegrityAudit
```

`RepairRegistry` применяет validator и при runtime reads/lookups, поэтому post-boot corruption не должен молча превратиться в malformed JSON API output.

Performance invariant: strict syntax validation выполняется на authoritative outer pass. В уже существующих O(n²)/O(n*m) duplicate/reference scans full JSON parse намеренно не дублируется; repeated scans остаются identity-focused.

Это persistence correctness hardening. Physical START, SSR boundary, reboot behavior и manual wire writeoff semantics не менялись.

## Последние code commits — JSON hardening

```text
9ddabf613f1edf95dc1da55cbba8763414e47968  Add flat persisted JSON syntax validator
96c863b1a1bde3a3725596940e74805da2c69111  Reject malformed flat JSON in business backup audit
d13269bb481d056623569b9ecdf91708be6b0b8b  Fail closed on malformed workshop JSON
07b20e9b88012446fcbc813e78b39349ecd70753  Reject malformed flat JSON in pricing audit
b86794238cab420d1d97c3b281fa233b92ccf317  Avoid repeated JSON parsing in business identity scans
86b19f35cad6c383377dee9c342e58f4978b0e79  Avoid repeated JSON parsing in registry duplicate scans
16f39b33cccaeed533c39c2e0144d6942169b2c7  Reject malformed flat JSON in material persistence audit
b3fd050c5e917691877e1c245fadde333742eed7  Reject malformed flat JSON in warehouse persistence audit
b7b362bfe1813f27eab0c904dc9c7fc4489e6f9e  Reject malformed flat JSON in movement audit
ab0b1f6b0381641e811fed5a18ac412ebb0667d2  Reject malformed flat JSON in conductor settings audit
090acf40fc4470c7b8719975df7f4ce218a3cdec  Keep pricing reference scans identity focused
```

## Предыдущие observability commits

```text
4a30e4ca08e1d2e010dded1ab3e93073f9ecaeed  Expose persistent allocator audit metrics
b38bb3b5190bb99d261f5552cecedfea4048289b  Return validated allocator high-water mark
52fae7716034ccacebc41f1f11715f5eebf193c2  Expose allocator high-water mark in backup manifest
1470b866c0b91aee4bd8dff1eddc6c26926be578  Expose winding session byte totals
cacdffa9ec822ad1425d6a4de34c10f836fbbab0  Measure winding session persistence bytes
a0c83b08f64c05f0232d287146850f9e9fd37ce5  Expose winding session byte totals in backup manifest
96a1c5bc8c4a5cb7f5b672d290bbac23867429c5  Measure deep backup domain durations
```

Предыдущие same-pass observability уже включает material/business/winding/warehouse counts; не реализовывать их повторно.

## Static integration review

Подтверждено repository-level review:

- compatibility audit overloads сохранены;
- public metrics headers содержат нужные includes;
- `CM_BackupExportWeb.h` явно включает `Arduino.h`;
- per-domain timing использует `uint32_t` `millis()` subtraction;
- timing не меняет порядок audit и не добавляет SD scan;
- `CM_FlatJsonObjectValidator.h` header-only и использует уже доступный Arduino `String`;
- authoritative workshop/pricing/material/warehouse/settings readers теперь fail closed на malformed flat JSON;
- nested identity/reference scans не получили full-parser CPU multiplier;
- `BackupActivityGuard::Safe` gating сохранён;
- winding `validateAll()` и session authoritative deep audit не заменены telemetry/hardening логикой;
- safety boundary physical START/SSR/manual writeoff не затронута.

Это **не** доказательство GREEN ESP32 build.

## Известные performance hotspots

Repository review показывает:

1. `MaterialPersistenceIntegrityAudit::check()` после local material scans транзитивно вызывает broad `WorkshopPersistenceIntegrityAudit::check()` + pricing audit, а backup позже повторяет часть domains.
2. `BackupBusinessDataIntegrityAudit` использует повторные uniqueness/reference scans.
3. Warehouse reference validation повторно ищет spool/repair references.
4. Backup preliminary session-directory scan выполняется до authoritative deep session persistence audit.

Это не correctness bugs. Per-domain timings позволяют измерить цену каждого пути напрямую.

## Что измерить на hardware/E2E стенде

Сохранить один `/api/backup/manifest` после полного production flow и снять:

```text
items[].size_bytes
все 29 Stage 0 metrics
snapshot_stability_duration_ms
```

Сначала выбрать самый дорогой `*_duration_ms`, затем сопоставить его с counts/bytes/high-water.

После фактических измерений решать:

- bounded in-request index;
- duplicate-audit decomposition;
- bounded rotation immutable histories.

До измерений не начинать database migration.

## Точная следующая repo-only точка

Stage 0 observability достаточно детализирован. Если hardware пока недоступен, не добавлять новые metrics ради количества.

Repo-only работа оправдана только при конкретной correctness/compile причине. Текущий следующий review-кандидат — runtime consumers append-only histories (например costing/report paths), где ещё может оставаться старый `{...}` shortcut после отдельного integrity audit. Ужесточать только фактически найденные readers, без изменения формул, provenance или mutation semantics.

## Обязательный следующий внешний этап

```text
linked repair
→ exact spool
→ JOB_ACK
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ manual wire writeoff
→ costing
→ finalization preflight
→ CLOSED
→ stable backup
```

Плюс fault cases:

- reboot/manual-review;
- microSD loss;
- corrupted persistence;
- UART faults;
- duplicate writeoff;
- close without wire coverage;
- backup during active winding.

## CI

Последняя подтверждённая compile failure была missing namespace closing brace в `CM_MaterialLedger.cpp`, исправленная commit:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8
```

Новые commits не считать GREEN без фактического Actions result. Connector может не видеть push-triggered runs; отсутствие результата не является GREEN.

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
