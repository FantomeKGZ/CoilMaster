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
- backup/run-level HTTP semantics audit.

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

Последний code commit:

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

## Repo-reviewable integration status

Подтверждено на уровне static repository review:

- `CM_WindingPersistenceIntegrityAudit` использует authoritative `WindingJournalQuery::validateAll()` + отдельный transition audit;
- `CM_WindingSessionPersistenceIntegrityAudit` остаётся authoritative deep parser/cross-file identity audit;
- compatibility `check(storage)` overloads сохранены;
- allocator high-water, record counts и session bytes не добавляют telemetry-only full scans;
- per-domain timing использует `uint32_t` `millis()` subtraction вокруг существующих вызовов;
- `CM_BackupExportWeb.h` явно включает `Arduino.h`, поэтому timing/String/F types доступны напрямую;
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

Если стенд пока недоступен, repo-only код допустим только как ещё одна same-pass observability существующего authoritative validator без дополнительного full scan либо как fix фактически подтверждённой compile/correctness проблемы.

## Hardware E2E — обязательно отдельно

Repository review и CI не доказывают физический ESP32 + Arduino path. Проверить:

```text
linked repair → exact spool → JOB_ACK → physical START
→ RUN_STARTED → RUN_COMPLETED → manual wire writeoff
→ costing → finalization preflight → CLOSED → stable backup
```

И fault cases: reboot/manual-review, microSD loss, corrupted persistence, UART faults, duplicate writeoff, close without wire coverage, backup during active winding.

Safety invariants не менять: no automatic physical START, no auto-resume, no direct ESP32/web SSR control, no automatic wire writeoff on `RUN_COMPLETED`, corruption/storage loss fail closed.
