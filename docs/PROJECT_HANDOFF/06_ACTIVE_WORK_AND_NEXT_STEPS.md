# Где остановились и что делать дальше

Дата обновления: 2026-08-08  
Ветка: `cmp-protocol-v1`

Код ветки всегда выше документации по приоритету. Полный snapshot: `docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md`.

## Уже закрыто — не повторять

Production flow собран:

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual wire writeoff
→ source_session_id + source_run_id → materials/pricing
→ finalization preflight → CLOSED → archive/report → read-only backup
```

Также уже закрыты:

- persistent allocator/session state/recovery;
- strict repair/motor/coil_program linkage;
- exact spool selection и run-level provenance;
- winding history + transition validation;
- warehouse/material recovery и costing;
- finalization coverage;
- whitelist backup/export + deep persistence integrity;
- winding `validateAll()` cleanup;
- backup/run-level HTTP semantics audit;
- Stage 0 backup observability до 29 metrics;
- flat persisted JSON syntax hardening для workshop/pricing/material/warehouse/settings.

## CI status

Последняя ранее подтверждённая ошибка Actions была missing closing brace в `CM_MaterialLedger.cpp`; fix:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8  Fix MaterialLedger namespace closure
```

Новые commits не считать GREEN без фактического build result. Для текущего head GREEN CI пока не подтверждён.

## Deep backup contract

Deep audit выполняется только при `BackupActivityGuard::Safe`. При active winding тяжёлый scan не запускается, export блокируется, stability/observability metrics остаются `null`.

Safe `snapshot_stable=true` означает проверку static whitelist, recovery markers, allocator/settings, workshop/pricing/material/warehouse references, winding schema/transitions и содержимого всех snapshot/state/spool-selection session files.

## Stage 0 performance observability

Manifest теперь возвращает **29 metrics без отдельного telemetry scan**.

Per-domain timing:

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

Population/high-water/size:

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

Timing оборачивает только уже существующие audit calls через `millis()`; filesystem I/O ради duration не добавляется. Если audit дошёл до domain, его duration публикуется даже при failure этого domain, а последующие неисполненные domains остаются `null`. Это позволяет локализовать slow failure.

`winding_session_directory_scan_duration_ms` отдельно измеряет уже существующий preliminary directory scan, а `winding_session_persistence_audit_duration_ms` — authoritative deep parser/cross-identity audit. До benchmark эти passes не объединять.

Record counts, allocator high-water и session file counts/bytes также собираются внутри уже выполняемых authoritative passes. Старые `check(storage)` contracts сохранены совместимо. Partial domain counts/high-water после failed domain audit наружу не публикуются.

Session byte totals являются telemetry-only: при 32-bit aggregate overflow только total-byte fields становятся `null`; integrity result не меняется.

Ключевой timing commit:

```text
96a1c5bc8c4a5cb7f5b672d290bbac23867429c5  Measure deep backup domain durations
```

Ключевые предыдущие observability commits:

```text
4a30e4ca08e1d2e010dded1ab3e93073f9ecaeed  Expose persistent allocator audit metrics
b38bb3b5190bb99d261f5552cecedfea4048289b  Return validated allocator high-water mark
52fae7716034ccacebc41f1f11715f5eebf193c2  Expose allocator high-water mark in backup manifest
1470b866c0b91aee4bd8dff1eddc6c26926be578  Expose winding session byte totals
cacdffa9ec822ad1425d6a4de34c10f836fbbab0  Measure winding session persistence bytes
a0c83b08f64c05f0232d287146850f9e9fd37ce5  Expose winding session byte totals in backup manifest
```

## Flat persisted JSON correctness hardening

Repository review после Stage 0 подтвердил отдельную correctness-причину: несколько persisted flat-NDJSON readers проверяли только внешние `{...}` и выбранные поля. Синтаксически повреждённая строка с ещё читаемым ID могла пройти часть integrity/startup checks или попасть в read-only JSON API.

Добавлен общий header-only:

```text
firmware/esp32/src/CM_FlatJsonObjectValidator.h
```

Он проверяет синтаксис уже прочитанного flat JSON object без нового SD pass и без внешней JSON dependency.

Hardened authoritative readers:

```text
CM_BackupBusinessDataIntegrityAudit
CM_RepairRegistry
CM_RepairPricingIntegrityAudit
CM_MaterialPersistenceIntegrityAudit
CM_WarehousePersistenceIntegrityAudit
CM_WarehouseMovementIntegrityAudit
CM_ConductorSettingsIntegrityAudit
```

Strict parser выполняется один раз на authoritative outer pass. В известных O(n²)/O(n*m) duplicate/reference scans повторный full JSON parse намеренно убран/не добавлен, чтобы correctness fix не создавал искусственный performance regression.

Ключевые commits:

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

Safety invariants не затронуты: это только fail-closed persistence parsing.

## Repo-reviewable integration status

Подтверждено на уровне static repository review:

- `CM_WindingPersistenceIntegrityAudit` использует authoritative `WindingJournalQuery::validateAll()` + отдельный transition audit;
- `CM_WindingSessionPersistenceIntegrityAudit` остаётся authoritative deep parser/cross-file identity audit;
- compatibility `check(storage)` overloads сохранены;
- allocator high-water, record counts и session bytes не добавляют telemetry-only full scans;
- per-domain timing использует `uint32_t` `millis()` subtraction вокруг существующих вызовов;
- `CM_BackupExportWeb.h` явно включает `Arduino.h`, поэтому timing/String/F types доступны напрямую;
- shared flat-JSON validator header-only и использует уже доступный Arduino `String`;
- workshop/material/warehouse/settings authoritative passes теперь отвергают malformed flat JSON;
- repeated identity/reference scans не получили лишнего full-JSON parser multiplier;
- `BackupActivityGuard::Safe` gating сохранён;
- safety semantics физического START/SSR/manual writeoff не изменены.

Это static repository review, а не доказательство успешной ESP32 сборки или hardware behavior.

## Подтверждённые performance hotspots для измерения

Repository review показывает:

- `MaterialPersistenceIntegrityAudit::check()` транзитивно вызывает broad `WorkshopPersistenceIntegrityAudit::check()` + pricing audit, а backup затем проверяет часть domains снова;
- `BackupBusinessDataIntegrityAudit` использует повторные uniqueness/reference lookups;
- warehouse reference validation повторно ищет spool/repair references;
- backup делает preliminary session-directory scan до authoritative deep session audit.

Это не correctness bugs. Новые per-domain durations позволяют сначала измерить цену каждого пути.

## Следующее практическое действие

Обязательный следующий этап — реальный hardware E2E ESP32 + Arduino и одновременный benchmark backup manifest. Сохранить один manifest с:

```text
items[].size_bytes
всеми 29 Stage 0 metrics
snapshot_stability_duration_ms
```

Сначала выбрать самый дорогой domain по `*_duration_ms`, затем объяснить его рост через record counts/session bytes/high-water. Только после этого решать bounded in-request index, duplicate-audit decomposition или rotation.

Не вводить rotation threshold, persistent optimistic cache, Stage 1 duplicate-scan refactor или database migration до этих данных, если не появится отдельная correctness-причина.

Если стенд пока недоступен, repo-only код допустим как fix фактически подтверждённой compile/correctness проблемы. Stage 0 observability уже достаточно детализирован; не добавлять метрики ради количества.

## Hardware E2E — обязательно отдельно

Repository review и CI не доказывают физический ESP32 + Arduino path. Проверить:

```text
linked repair → exact spool → JOB_ACK → physical START
→ RUN_STARTED → RUN_COMPLETED → manual wire writeoff
→ costing → finalization preflight → CLOSED → stable backup
```

И fault cases: reboot/manual-review, microSD loss, corrupted persistence, UART faults, duplicate writeoff, close without wire coverage, backup during active winding.

Safety invariants не менять: no automatic physical START, no auto-resume, no direct ESP32/web SSR control, no automatic wire writeoff on `RUN_COMPLETED`, corruption/storage loss fail closed.
