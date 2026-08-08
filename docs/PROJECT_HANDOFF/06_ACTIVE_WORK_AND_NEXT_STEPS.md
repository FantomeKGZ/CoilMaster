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

Deep audit выполняется только при `BackupActivityGuard::Safe`. При active winding тяжёлый scan не запускается, export блокируется, stability/count metrics остаются `null`.

Safe `snapshot_stable=true` означает проверку static whitelist, recovery markers, allocator/settings, workshop/pricing/material/warehouse references, winding schema/transitions и содержимого всех snapshot/state/spool-selection session files.

## Stage 0 performance observability

Без отдельного telemetry scan manifest теперь возвращает:

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
warehouse_movement_record_count
```

Material/business/winding/warehouse record counts и session file counts собираются внутри уже выполняемых authoritative validation passes. Старые `check(storage)` contracts сохранены совместимо там, где добавлены metrics overloads. Partial metrics после failed audit наружу не публикуются.

Session file counters считаются внутри существующих deep parser/cross-identity проходов `CM_WindingSessionPersistenceIntegrityAudit`; отдельного обхода directory ради telemetry не добавлено. Отсутствующая snapshot/state/spool-selection directory при успешном audit даёт `0`.

Business counters считаются внутри уже существующих `CM_BackupBusinessDataIntegrityAudit` passes: client/motor/repair counts — в uniqueness scans, repair-status/pricing counts — в текущих parser/reference scans. Отдельного full-file чтения ради telemetry не добавлено; отсутствующий файл при успешном audit даёт `0`.

Последнее расширение:

```text
9c33178d8b580460e1d34962322fe81b9771dccc  Expose winding session persistence counts
afd2c9e3df2e63b59553e4f10e12eb4d2199e46d  Count winding session persistence files
abc4b02ef284ed86fdfc3e31149ccf8adf9d5e8b  Expose winding session file counts in backup manifest
7cb635d4088d38cef58dcb39bc170bf7f55dd0c7  Document winding session backup observability
cf7df132d190bd359a4f4b85b2553f6dcdba5dd4  Expose business data audit counts
33746bf301ee8a31417361ce6fd8f7a2ce1635f7  Count business backup audit records
1871d140e1e493b6e64ada3502eaa5fbcb75f0f6  Expose business audit counts in backup manifest
702edf98c9b993332fdaf17aa2359c1128391f27  Document business backup observability
```

## Repo-reviewable integration status

Подтверждено на уровне static repository review:

- `CM_WindingPersistenceIntegrityAudit` уже использует authoritative `WindingJournalQuery::validateAll()` + отдельный `WindingJournalTransitionAudit::validate()`; cursor-pagination full scan отсутствует;
- `CM_WindingSessionPersistenceIntegrityAudit` остаётся authoritative deep parser/cross-file identity audit и не дублируется;
- старый `WindingSessionPersistenceIntegrityAudit::check(storage)` сохранён и делегирует metrics overload;
- session counts увеличиваются только после успешного parser/load + identity validation соответствующего файла;
- старый `BackupBusinessDataIntegrityAudit::check(storage)` сохранён и делегирует metrics overload;
- business counts увеличиваются только внутри уже существующих successful validation passes и копируются наружу только после полного успешного business audit;
- public metrics headers явно включают `FS.h` и `stdint.h`;
- `CM_BackupExportWeb.cpp` использует оба metrics overload и сохраняет `BackupActivityGuard::Safe` gating;
- partial business/session counts не публикуются при failed audit;
- safety semantics и порядок физических действий не изменены.

Это static repository review, а не доказательство успешной ESP32 сборки или hardware behavior.

## Подтверждённый performance hotspot

Repository review показал, что полный `MaterialPersistenceIntegrityAudit::check()` после собственных material scans транзитивно вызывает:

```text
WorkshopPersistenceIntegrityAudit::check()
RepairPricingIntegrityAudit::check()
```

А workshop audit снова проходит warehouse, allocator, session persistence, winding schema и transitions. Backup orchestration позже проверяет эти domains отдельно. Это реальный duplicate I/O path, но не correctness bug.

Кроме того, `BackupBusinessDataIntegrityAudit` намеренно использует повторные uniqueness/reference lookups. Новые `workshop_*_record_count` и `repair_pricing_record_count` позволяют сопоставить эту стоимость с реальным размером данных до любого bounded-index refactor.

До benchmark не делать Stage 1 refactor только ради эстетики. Если измерения покажут заметную цену, безопасный следующий refactor — разделить local material validation и cross-domain dependency validation так, чтобы backup выполнял каждый authoritative domain audit один раз, сохранив public contracts.

## Следующее практическое действие

Обязательный следующий этап — реальный hardware E2E ESP32 + Arduino и одновременный benchmark backup manifest. На стенде снять вместе:

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
warehouse-movements.size_bytes
warehouse_movement_record_count
snapshot_stability_duration_ms
```

После измерений выбирать hotspot по фактической latency/size/population. Не вводить rotation threshold, persistent optimistic cache, Stage 1 duplicate-scan refactor или database migration до этих данных, если не появится отдельная correctness-причина.

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
