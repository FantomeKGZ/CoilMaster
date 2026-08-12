# Hardware backup manifest baseline — 2026-08-12

Ветка: `cmp-protocol-v1`

Этот checkpoint получен с реально прошитого ESP32 после успешного clean PlatformIO build текущего firmware.

## Build checkpoint

```text
RAM:   14.4% (used 47320 bytes from 327680 bytes)
Flash: 86.7% (used 1136229 bytes from 1310720 bytes)
SUCCESS Took 31.04 seconds
```

Это подтверждает local compile/link. GitHub CI отдельно не подтверждён.

## Backup manifest runtime checkpoint

ESP32 `/api/backup/manifest` на реальной microSD вернул:

```text
export_allowed=true
activity_state_verified=true
snapshot_stability_checked=true
snapshot_stable=true
snapshot_stability_reason=null
snapshot_stability_duration_ms=1429
```

То есть read-only backup/deep-integrity runtime path реально прошёл без integrity failure.

## Per-domain timings

```text
persistent_id_audit_duration_ms=74
conductor_settings_audit_duration_ms=23
material_persistence_audit_duration_ms=634
business_data_audit_duration_ms=58
autonomous_winding_archive_audit_duration_ms=34
winding_persistence_audit_duration_ms=55
warehouse_persistence_audit_duration_ms=25
warehouse_movements_audit_duration_ms=null
winding_session_directory_scan_duration_ms=93
winding_session_persistence_audit_duration_ms=376
```

На этом почти пустом dataset самые дорогие измеренные части:

```text
material_persistence = 634 ms
winding_session_persistence = 376 ms
session_directory scan = 93 ms
persistent id = 74 ms
```

Не делать Stage 1 performance refactor только по этому baseline: dataset почти пустой и не показывает scaling behaviour. Нужен populated production-style E2E benchmark.

`warehouse_movements_audit_duration_ms=null` корректен: `warehouse-movements.ndjson` на этой карте ещё отсутствует, поэтому этот optional audit не выполнялся.

## Record counters

```text
winding_allocator_last_id=1
material_catalog_record_count=0
material_usage_record_count=0
material_adjustment_record_count=0
workshop_client_record_count=0
workshop_motor_record_count=0
workshop_repair_record_count=0
repair_status_record_count=0
repair_pricing_record_count=0
autonomous_winding_event_record_count=0
autonomous_winding_started_record_count=0
autonomous_winding_completed_record_count=0
autonomous_winding_assignment_record_count=0
winding_journal_record_count=1
winding_snapshot_file_count=1
winding_state_file_count=1
winding_spool_selection_file_count=0
winding_snapshot_total_bytes=290
winding_state_total_bytes=177
winding_spool_selection_total_bytes=0
warehouse_spool_record_count=0
warehouse_price_record_count=0
warehouse_movement_record_count=null
```

## Autonomous archive verification

Manifest whitelist содержит оба новых logical items:

```text
autonomous-winding-events
→ /data/autonomous-windings/events.ndjson

autonomous-winding-assignments
→ /data/autonomous-windings/assignments.ndjson
```

В момент этого checkpoint оба файла ещё отсутствовали и все autonomous counters были `0`. Это допустимое empty-state и не является integrity failure: autonomous audit прошёл за 34 ms, а `snapshot_stable=true`.

Следующий local Arduino task после текущего ESP32 firmware должен создать `events.ndjson`. `assignments.ndjson` появляется только после ручной assignment completed autonomous task к motor.

## Whitelist runtime count

Manifest сообщил:

```text
count=17
```

Из уже существующих файлов на этом checkpoint:

```text
/data/winding-runs/events.ndjson            150 bytes
/data/winding-jobs/id-state.txt              49 bytes
/data/winding-jobs/id-state.bak              49 bytes
```

Остальные business/warehouse/material/autonomous files ещё не были созданы.

## Следующий E2E checkpoint

Заполнить реальный production-style dataset и пройти:

```text
client
→ motor
→ OPEN repair
→ costing
→ exact ACTIVE spool
→ linked winding
→ JOB_ACK
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ manual exact-run wire writeoff
→ finalization preflight
→ CLOSED
→ stable backup manifest
```

После этого сохранить второй manifest и сравнить durations/counts/bytes с этим baseline. Только по измеренному росту выбирать bounded index/decomposition/rotation. Database migration и arbitrary rotation threshold до измерений не вводить.

Safety invariants остаются неизменны:

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web не управляют SSR напрямую;
- `RUN_COMPLETED` сам не списывает провод;
- wire writeoff ручной и exact `spool_id + source_session_id + source_run_id`.
