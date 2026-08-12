# Controlled recovery and autonomous archive scaling — 2026-08-12

## Scope

This checkpoint follows the hardware-confirmed production E2E, active-winding backup negative test, runtime storage corruption hardening, and UART timeout/replay hardening.

Safety invariants remain unchanged:

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never controls SSR directly;
- `RUN_COMPLETED` alone never writes off wire;
- wire writeoff remains manual and bound to exact `spool_id + source_session_id + source_run_id`.

## Controlled recovery hardening

Recovery closure is now fail-closed around the full persisted identity, not only the runtime state file.

The operator recovery endpoints now revalidate immediately before closure:

1. latest persisted job state;
2. immutable job snapshot and exact `job_id/session_id` identity;
3. for linked jobs, immutable spool selection and exact `job_id/session_id/repair_id/motor_id` identity.

Only after those checks may `CLOSED_AFTER_REVIEW` be persisted. The response reports restart and ESP32 restarts intentionally so stale in-memory active-job state cannot survive a completed review.

The legacy `/api/recovery/acknowledge` path was brought to the same semantics as `/api/recovery/acknowledge-and-restart`; it is no longer a weaker bypass.

During boot/runtime recovery a linked job is no longer considered fully recoverable if its immutable spool-selection record is unavailable, malformed, or mismatched. New job creation stays blocked.

## Backup export guard

`BackupActivityGuard` no longer treats a runtime `Safe` result as sufficient proof for direct file export.

For an inactive persisted job it also revalidates the latest immutable snapshot. For a linked job it additionally revalidates the exact immutable spool selection. If that identity cannot be proven, backup export reports activity state unavailable and remains blocked.

This is important because `/api/backup/file` intentionally does not execute the complete deep audit for every individual file download.

## Autonomous archive deep-audit scaling

The read-only `AutonomousWindingArchive::validateStorage()` was rewritten to remove the previous quadratic full-file rescans.

### Event audit

`events.ndjson` is now validated in one forward pass using the protocol guarantees already enforced by Arduino:

- LOCAL_EVT transport is FIFO;
- `run_id` is monotonically allocated;
- a normal `RUN_COMPLETED` follows its matching `RUN_STARTED` for the same run;
- a recovered completion without a recorded START remains valid only with `start_observed=0` (Arduino EEPROM recovery case).

The one-pass audit also verifies matching program semantics for START/COMPLETE pairs and rejects duplicate/conflicting run identities.

### Assignment audit

`assignments.ndjson` still permits operator assignments to historical runs in arbitrary order, so a direct merge join is not possible. Cross-reference validation is therefore batched in fixed groups of 32 assignments. Each batch performs one event-file scan instead of one event-file scan per assignment.

The storage format is unchanged: source data remains append-only NDJSON. No records are deleted, compacted, rotated, or migrated to a database in this checkpoint.

## Build / CI state

The previously reported ESP32 hardware build success predates the controlled-recovery and archive-scaling changes in this checkpoint.

Therefore for the current HEAD:

- **BUILD NOT CONFIRMED**
- **CI NOT CONFIRMED**

Next required checkpoint is one clean ESP32 build, followed by a normal stable backup manifest check. On populated data, compare `autonomous_winding_archive_audit_duration_ms` with the earlier baseline before selecting any rotation threshold.
