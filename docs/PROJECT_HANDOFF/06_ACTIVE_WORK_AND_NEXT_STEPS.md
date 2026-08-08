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

Новые commits не считать GREEN без фактического build result.

## Deep backup contract

Deep audit выполняется только при `BackupActivityGuard::Safe`. При active winding тяжёлый scan не запускается, export блокируется, stability/count metrics остаются `null`.

Safe `snapshot_stable=true` означает проверку static whitelist, recovery markers, allocator/settings, workshop/pricing/material/warehouse references, winding schema/transitions и содержимого всех snapshot/state/spool-selection session files.

## Stage 0 performance observability

Без дополнительного persistence scan manifest теперь возвращает:

```text
snapshot_stability_duration_ms
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
winding_journal_record_count
warehouse_movement_record_count
```

Material counters считаются внутри уже существующих `materials.ndjson`, `usage.ndjson`, `adjustments.ndjson` scans. Старый `MaterialPersistenceIntegrityAudit::check(storage)` сохранён; добавлен совместимый metrics overload. Если audit успешен и конкретный файл отсутствует, count равен `0`, а наличие видно через `items[].exists`.

Warehouse/winding counters также считаются в существующих authoritative passes и не создают второй scan.

Текущий batch:

```text
63614fe363adaf912fdf35775ecff6befad34ed6  Expose warehouse movement audit count
fe024d6908e4488e114b633c97d06848d2d9bc38  Count warehouse movement audit records
a78cf149dd5d1f588988ddda3e2d046459fd36b5  Expose warehouse movement audit count
ac031d8cc14a74786e10c8adb782776b0d16e97f  Expose material persistence audit counts
6cf4ad7da157c8e65f131b9a851c4243c0914e31  Count material persistence audit records
38befe338cfc57879d2ad09fc6be54d54c190441  Expose material audit counts in backup manifest
0f10ed32d110c28b21af7c46c6be20a084c6ba2b  Document material backup observability
```

## Подтверждённый performance hotspot

Repository review показал, что полный `MaterialPersistenceIntegrityAudit::check()` после собственных material scans транзитивно вызывает:

```text
WorkshopPersistenceIntegrityAudit::check()
RepairPricingIntegrityAudit::check()
```

А workshop audit снова проходит warehouse, allocator, session persistence, winding schema и transitions. Backup orchestration позже проверяет эти domains отдельно. Это означает реальный duplicate I/O path, но пока не является причиной менять storage format.

До benchmark не делать Stage 1 refactor только ради эстетики. Если измерения покажут заметную цену, следующий безопасный refactor — разделить local material validation и cross-domain dependency validation так, чтобы backup выполнял каждый authoritative domain audit один раз, сохранив старые public contracts для других callers.

## Следующее практическое действие

На стенде снять вместе:

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

После измерений выбирать один hotspot по фактической latency/size. Не вводить rotation threshold, persistent optimistic cache или database migration до этих данных.

Если стенд пока недоступен, следующий repo-only код допустим только как same-pass observability существующего authoritative validator без дополнительного full scan.

## Hardware E2E — обязательно отдельно

Repository review и CI не доказывают физический ESP32 + Arduino path. Проверить:

```text
linked repair → exact spool → JOB_ACK → physical START
→ RUN_STARTED → RUN_COMPLETED → manual wire writeoff
→ costing → finalization preflight → CLOSED → stable backup
```

И fault cases: reboot/manual-review, microSD loss, corrupted persistence, UART faults, duplicate writeoff, close without wire coverage, backup during active winding.

Safety invariants не менять: no automatic physical START, no auto-resume, no direct ESP32/web SSR control, no automatic wire writeoff on `RUN_COMPLETED`, corruption/storage loss fail closed.
