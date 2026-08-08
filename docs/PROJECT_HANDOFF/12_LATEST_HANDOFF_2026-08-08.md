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
- flat persisted JSON syntax hardening для deep audits и основных runtime/history/writeoff readers.

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

Manifest возвращает 29 runtime metrics без дополнительного telemetry full scan.

Total/per-domain durations:

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

Population/high-water/bytes:

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

Observability invariants:

- timing оборачивает уже существующие audit calls через `millis()`;
- filesystem I/O ради telemetry не добавляется;
- если audit дошёл до domain и тот завершился failure, его duration сохраняется, последующие неисполненные domains остаются `null`;
- session directory scan и deep session persistence audit измеряются отдельно;
- compatibility `check(storage)` overloads сохранены;
- partial domain counts/high-water не публикуются;
- session aggregate byte overflow является telemetry-only и не меняет integrity result.

До hardware benchmark не вводить arbitrary rotation threshold, persistent optimistic cache, database migration или Stage 1 duplicate-scan refactor без отдельной correctness-причины.

## Flat persisted JSON correctness hardening

Repository review выявил correctness gap: часть flat NDJSON readers проверяла строку только по внешним `{...}` и выбранным полям. Синтаксически повреждённая строка с читаемым ID могла пройти часть startup/deep/runtime checks или попасть в JSON API/агрегаты.

Добавлен общий header-only validator:

```text
firmware/esp32/src/CM_FlatJsonObjectValidator.h
```

Он проверяет уже прочитанный flat JSON object без внешней JSON dependency и без нового SD pass.

### Deep/startup readers

Hardened:

```text
CM_BackupBusinessDataIntegrityAudit
CM_RepairRegistry
CM_RepairPricingIntegrityAudit
CM_MaterialPersistenceIntegrityAudit
CM_WarehousePersistenceIntegrityAudit
CM_WarehouseMovementIntegrityAudit
CM_ConductorSettingsIntegrityAudit
```

### Runtime/history/costing/manual-writeoff readers

Дополнительно hardened:

```text
CM_RepairCosting.cpp
CM_MaterialHistory.cpp
CM_MaterialUsageHistory.cpp
CM_MaterialAdjustment.cpp
CM_MaterialLedgerCurrency.cpp
CM_RepairPricingHistory.cpp
CM_WarehouseWriteOffHistory.cpp
CM_WarehouseStore.cpp
CM_WarehouseWriteOff.cpp
CM_WarehousePrice.cpp
CM_WarehouseSpoolIdentity.cpp
```

Практический эффект:

- malformed persisted history не должен попадать в material/pricing/write-off JSON output;
- costing не строит totals поверх malformed movement/usage/pricing rows;
- material adjustment/recovery блокируется до temp swap/replay при malformed source/pending/audit JSON;
- material currency lookup fail-closed;
- warehouse summary и ID allocation fail-closed;
- manual writeoff проверяет movement/spool state до PENDING transaction и atomic spool rewrite;
- rewritten spool/material-adjustment row проверяется до temp-file write;
- exact active spool identity и warehouse price проверяются непосредственно в runtime preflight.

Performance invariant: strict parser выполняется на authoritative/main row pass. В уже существующих O(n²)/O(n*m) duplicate/reference scans full JSON parse намеренно не дублируется.

Safety boundary не менялся: physical START только физический, SSR остаётся у Arduino, auto-resume отсутствует, `RUN_COMPLETED` не выполняет wire writeoff, списание остаётся ручным и связано с exact spool/session/run provenance.

## Ключевые JSON-hardening commits

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
02a80ea88a157ebfaee389d9c87368b36235ccbd  Fail closed on malformed costing histories
4d1761aa7b2b958a880b7de2b30c3bc8b09e62cd  Reject malformed pricing history JSON
9fb8918eb8fad0c939cf04330ddfce81073c1f8c  Reject malformed write-off history JSON
b43568c4231d9c972ed92f0b3b23c038837e3252  Reject malformed material usage history JSON
8c3f2b76464918a889a0e5a42d27c6ac4ff82c63  Fail closed on malformed warehouse summary state
d5c0a99449132fd343e1dc04e92e4136471dfbbb  Fail closed on malformed write-off transaction state
ea37009b15cf5367d4c5712bdd412c16f4649827  Fail closed on malformed warehouse price state
e01b7f8e6219558622a1c86c517c4d200f767992  Fail closed on malformed spool identity state
```

Дополнительные material runtime commits для `CM_MaterialHistory.cpp`, `CM_MaterialAdjustment.cpp`, `CM_MaterialLedgerCurrency.cpp` находятся в текущей branch history; session-level запись — `docs/PROJECT_HANDOFF/10_SESSION_LOG.md`.

## Явно незакрытый repo-only кандидат

`firmware/esp32/src/CM_MaterialLedger.cpp` остаётся единственным явно подтверждённым крупным runtime gap этого прохода. В нём ещё есть direct persisted-row paths со старым shape/selected-field parsing:

```text
material catalog JSON output
material row scan before usage confirmation
pending usage recovery / durable usage lookup
direct stock lookup
generic next ID scan
quantity rewrite/restore source rows
```

Файл дважды fetched из `cmp-protocol-v1`; подготовленный большой contents update был остановлен GitHub connector safety-filter **до записи**. Low-level Git object workflow для обхода этого filter намеренно не использовался. Поэтому `CM_MaterialLedger.cpp` не считать hardened.

Следующий repo-only fix — только обычным SHA-guarded `update_file`, если connector пропускает его. Не заменять это broad integrity scan перед каждой mutation, поскольку это добавило бы лишний SD I/O и исказило performance contract.

## Static integration review

Подтверждено repository-level review:

- compatibility audit overloads сохранены;
- `CM_BackupExportWeb.h` явно включает `Arduino.h`;
- per-domain timing использует `uint32_t` `millis()` subtraction;
- `CM_FlatJsonObjectValidator.h` self-contained через `Arduino.h`, внешних deps нет;
- strict flat JSON validation не добавляет filesystem pass;
- nested identity/reference scans не получили parser multiplier;
- `BackupActivityGuard::Safe` gating сохранён;
- winding `validateAll()` и authoritative session deep audit не заменены;
- formulas/rounding/provenance/manual writeoff transaction semantics не изменены.

Это **не** доказательство GREEN ESP32 build.

## Известные performance hotspots

1. `MaterialPersistenceIntegrityAudit::check()` транзитивно вызывает broad `WorkshopPersistenceIntegrityAudit::check()` + pricing audit, а backup позже повторяет часть domains.
2. `BackupBusinessDataIntegrityAudit` использует повторные uniqueness/reference scans.
3. Warehouse reference validation повторно ищет spool/repair references.
4. Backup preliminary session-directory scan выполняется до authoritative deep session persistence audit.

Это не correctness bugs. Per-domain timings позволяют измерить цену каждого пути напрямую.

## Что измерить на hardware/E2E стенде

Сохранить один `/api/backup/manifest` после полного production flow:

```text
items[].size_bytes
все 29 Stage 0 metrics
snapshot_stability_duration_ms
```

Сначала выбрать самый дорогой `*_duration_ms`, затем сопоставить его с counts/bytes/high-water. Только после фактических измерений решать bounded in-request index, duplicate-audit decomposition или bounded rotation immutable histories.

До измерений не начинать database migration.

## Точная следующая repo-only точка

1. `CM_MaterialLedger.cpp` — только если обычный SHA-guarded update проходит connector safety-filter.
2. После этого продолжать только по реально найденным split runtime/recovery readers с подтверждённой correctness-причиной; не угадывать пути.
3. Stage 0 не расширять metrics ради количества.
4. Stage 1 duplicate-scan/rotation/database work не начинать до benchmark без отдельной correctness-причины.

## Обязательный внешний этап

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
