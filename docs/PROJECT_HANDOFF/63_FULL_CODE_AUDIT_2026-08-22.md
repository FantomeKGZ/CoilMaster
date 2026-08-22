# Checkpoint 63 — Full code audit phase

Date: 2026-08-22  
Branch: `cmp-protocol-v1`

## Purpose

This checkpoint owns the active full-code audit. Phase 9 Web implementation is complete in checkpoint 64. The targeted JOB/UART desync review is summarized in checkpoint 65. ESP32 build-identity CI recovery is recorded in checkpoint 66.

Historical checkpoints are evidence, not an automatic task queue.

After the audit is complete, the next explicitly approved phase is a repository cleanup: remove only proven-unused folders/files, duplicated implementations, stale transition artifacts and dead code after dependency/build/test evidence shows they are not part of production or required history/tooling.

## Verification baseline

The operator explicitly reported **all visible workflows green** for branch HEAD:

```text
ef095a5eb05ae5f886020510ef11324d0f4882ad
Advance persistence audit past settings recovery
```

Treat that exact state as **USER CONFIRMED GREEN**. Any commit after this SHA requires a new exact-current verification or explicit operator confirmation.

## Audit scope

### A. Arduino safety and realtime ownership

Review `Core/`, `Arduino/`, `firmware/arduino/src/` for physical START/SSR/Hall ownership, repeat/run identity, remote JOB lifecycle, UART validation/retry/replay, RAM/Flash/stack, blocking paths and reboot/fault behavior.

### B. ESP32 runtime, persistence and APIs — CURRENT

Review `firmware/esp32/src/` for lifecycle/state consistency, persistence identities, workshop/warehouse/writeoff/costing, backup/restore, network/FTP/RTC/SD failure semantics, API bounds/overflow, atomic writes, NDJSON cost and duplicate ownership.

### C. Desktop/mobile Web

Review desktop/mobile/shared assets for API parity, error handling, unsafe optimistic state, paging/bounds, provenance, confirmations, semantic divergence and injection/escaping risks.

### D. Tests and CI

Review `Tests/`, workflows, `platformio.ini` and build scripts for real production coverage, stale/false-positive assertions, fail-open steps, source-filter omissions and trigger gaps.

### E. Documentation/AI routing

Review authoritative entrypoints for current-code accuracy and stale-task routing.

### F. Post-audit repository cleanup — AFTER A..E only

Build a dependency inventory before deleting anything. Classify candidates as:

```text
DELETE       proven unused and unreferenced by production/build/tests/docs/runtime assets
MERGE        duplicate implementations with one authoritative owner
KEEP         active source/tooling/history required by build, tests, docs or operation
REVIEW       uncertain dependency; do not delete until proven safe
```

Cleanup must not change safety behavior, persistence schema/provenance, required migration/history evidence or hardware ownership. Run applicable CI after cleanup and compare the final tree against the pre-cleanup baseline.

Initial inventory already exposes obvious low-risk candidates such as one-byte placeholder README files, but nothing is deleted until this phase begins.

## Severity

```text
P0 — immediate physical/data safety risk or destructive corruption path
P1 — serious functional/state/persistence bug likely to affect production
P2 — concrete robustness/performance/maintainability weakness
P3 — low-risk cleanup/dead code/docs/test-quality issue
```

Do not label speculative redesign or style preference as a defect.

## Findings status

### A-001..A-007 — FIXED

Remote JOB admission/parser/correlation, zero-id ALL_CLEAR recovery identity, stale-cancel handling, control/RUN ordering and late RUN after lost JOB_ACK have been hardened. Full UART evidence is in checkpoint 65. No physical START or SSR authority moved to ESP32/Web.

### B-001 — P1 — backup activity guard could promote unknown runtime to Safe — FIXED

`BackupActivityGuard::check()` requires runtime `Safe` before persisted identity checks. Runtime `Busy` stays Busy and runtime `Unavailable` stays Unavailable.

### B-002 — P1 — restore/apply lacked a global production-mutation interlock — FIXED

One process-wide restore lock blocks non-GET `/api/*` with `409 restore_mutation_active` during APPLY/rollback. GET/status remain available. Forward apply rechecks live activity and enters rollback on loss of proven Safe. APPLIED/FAILED remain locked until reboot/cleanup. Operator-only APPLY, no-auto-resume, physical START and SSR ownership remain unchanged.

### B-003 — P2 — network profile API conflated storage failures with client validation — FIXED

Network API distinguishes 400 validation, 404 absent profile, 409 capacity/conflict, 500 persistence/reload failure and 503 unavailable store/manager.

### B-004 — P1 — JobStateStore deleted authoritative state before replacement commit — FIXED

Current transaction is:

```text
write + verify temp
old target -> .bak
verified temp -> target
verify committed target
remove .bak only after verification
```

Interrupted `.bak/.tmp` evidence is preserved/fails closed instead of silently losing RUN/manual-review state. Regression: `Tests/Web/check_job_state_atomic_replace.js`.

### B-005 — P2 — linked JOB preparation partial transaction — FIXED / USER CONFIRMED GREEN at ef095a5e

Authoritative order:

```text
snapshot
CREATED + WAITING_DELIVERY + zero-run runtime state
exact spool selection when linked
DELIVERING runtime state
UART queueJob
```

`JobStateStore::isLocalPreparation()` is true only for exact CREATED/WAITING_DELIVERY/zero-run state. It may remain as immutable audit evidence and be superseded by a higher-ID job, but never auto-queues/resumes. DELIVERING/TIMED_OUT/accepted/running/fault remain fail-closed.

Regression: `Tests/Web/check_job_preparation_transaction.js`.

### B-006 — P2 — network profile recovery could promote uncommitted temp over committed backup — FIXED

Recovery prefers valid committed main, otherwise valid backup, and promotes temp only when no backup exists (interrupted first write). Invalid evidence fails closed. Regression: `Tests/Web/check_network_profile_atomic_recovery.js`.

### B-007 — P2 — remote backup settings recovery could promote uncommitted temp — FIXED / USER CONFIRMED GREEN at ef095a5e

Committed-first recovery prevents a brownout from silently applying FTP credentials, target directory, retention or schedule that `save()` never completed.

Regression: `Tests/Web/check_remote_backup_settings_atomic_recovery.js`.

### B-008 — P2 — conductor settings recovery could promote uncommitted temp — FIXED / USER CONFIRMED GREEN at ef095a5e

Committed-first recovery prevents an interrupted write from making uncommitted Al/Cu conversion ratios, deviation limits or maximum strand count authoritative after reboot.

Regression: `Tests/Web/check_conductor_settings_atomic_recovery.js`.

## Reviewed without a new production-data defect in this pass

- `PersistentIdAllocator`: verified temp, backup high-water and fail-closed interrupted transaction handling already present.
- `JobSnapshotStore`: immutable create-only final path; no destructive replace of an existing snapshot.
- `JobSpoolSelectionStore`: immutable create-only selection, bounded parse and exact session identity.
- `RepairRegistry`: append-only NDJSON; malformed/partial tail poisons readiness instead of deleting older records.
- `NetworkManager`: AP recovery is started before profile loading.
- `RtcClock`: NTP writes require live runtime Safe and are verified after DS3231 write.
- `WebRecoveryFtpServer`: scoped to `/web`, rechecks live activity and does not write production `/data`.

## Current active target

Continue section B only for concrete findings:

1. remaining mutable single-file stores for destructive swap/ambiguous recovery;
2. remaining backup/restore/activity-guard consistency;
3. resource/NDJSON hotspots only where evidence exists.

Then complete C Web, D tests/CI and E docs/AI routing. Only after A..E are complete move to F cleanup/de-duplication.

Do not reopen B-001..B-008 without a concrete regression.

## External hardware verification gate

Still required when the stand is available:

```text
normal JOB -> Arduino READY
physical START only
RUN_STARTED -> RUN_COMPLETED
repeat > 1 -> physical START each run
no automatic material writeoff
zero-run cancel / ALREADY_CLEAR / safe physical ALL_CLEAR
late zero-id ALL_CLEAR must not cancel fresh job
lost JOB_ACK -> TIMED_OUT/manual review -> late RUN_STARTED reconciliation
reboot waiting/running -> no auto resume

B-005 preparation boundary:
failed linked preparation before DELIVERING -> no JOB appears on Arduino
next higher-ID job may be created after reboot
DELIVERING/reboot still requires manual review

restore interlock:
GET status remains available during APPLY
POST/DELETE API mutation -> 409 restore_mutation_active
APPLIED requires reboot before mutations resume
```

## Execution rules / safety boundary

For every existing file changed: fetch current full `cmp-protocol-v1` content + blob SHA, make smallest safe fix, add regression coverage where practical, update current docs, and never claim exact-current GREEN without matching evidence or explicit operator confirmation.

Safety invariants remain unchanged:

```text
physical START only
no automatic START between repeats
no auto-resume after reboot
Arduino owns SSR
ESP32/Web never directly drive SSR
RUN_COMPLETED never auto-writes off material
manual writeoff uses exact source_session_id + source_run_id
spool_id optional only for approved KG_FIRST unallocated/manual path
exact spool provenance retained when a spool is used
backup restore operator-only, transactional and fail-closed
no automatic production-data cleanup
```

## Audit status

```text
Phase 9 implementation: COMPLETE (checkpoint 64)
Arduino findings A-001..A-007: FIXED
Targeted UART repo review: COMPLETE -> hardware gate retained
ESP32 B-001..B-008: FIXED at repo/source-contract level
HEAD ef095a5e: USER CONFIRMED GREEN
ESP32 remaining persistence/backup/resource audit: CURRENT
Web audit: NEXT after section B concrete findings exhausted
Tests/CI audit: PENDING
Documentation/AI consistency audit: PENDING
Final repo-wide recheck: PENDING
Post-audit cleanup/de-duplication: APPROVED / PENDING audit completion
```