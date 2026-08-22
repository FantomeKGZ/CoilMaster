# Checkpoint 63 — Full code audit phase

Date: 2026-08-22  
Branch: `cmp-protocol-v1`

## Purpose

This checkpoint owns the active full-code audit. Phase 9 Web implementation is complete in checkpoint 64. The targeted JOB/UART desync review is summarized in checkpoint 65. ESP32 build-identity CI recovery is recorded in checkpoint 66.

Historical checkpoints are evidence, not an automatic task queue.

## Verification baseline

The operator explicitly reported **all visible workflows green** when branch HEAD was:

```text
1bff98965c8608a66d269f51966a22fbd907047f
Advance ESP32 persistence audit queue
```

Treat that exact state as **USER CONFIRMED GREEN**. B-005 commits created after it remain **exact-current verification pending** until matching workflow results are inspected or explicitly confirmed.

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

### B-005 — P2 — linked JOB preparation partial transaction — FIXED / exact-current verification pending

Old order could leave immutable snapshot + exact spool selection without runtime state if the later state write failed. Blind cleanup was rejected because an ambiguous state write result must never delete provenance.

Implemented a durable pre-UART transaction boundary:

```text
65a8e982c85fbacbb0c430f20b664d9796848423  Define local job preparation state
dde7b1338a320f9af53ce55baaa0c7ffbebeb500  Allow safe local preparation rollover
5894da84e0c0133f54a2499fda81f1614f4a8013  Treat local preparation as inactive recovery
c5ac8c84618495e27c828cc5b7ec8e5b8c3a0d4e  Recognize safe local job preparation
be8a31d0f1a4ba8a16bb8dd1f40e4a80e59f9463  Commit job preparation before spool selection
a1da06c4d2c5a9c148a3d1c4669c6d87f4be8744  Guard job preparation transaction boundary
bdfee58d83b6d18663dfbf9db88604a952bee34a  Run job preparation transaction audit
```

Authoritative order is now:

```text
allocate IDs
immutable snapshot
CREATED + WAITING_DELIVERY + zero-run runtime state
exact spool selection when linked
DELIVERING runtime state
UART queueJob
```

`JobStateStore::isLocalPreparation()` is the shared predicate for exactly:

```text
delivery = CREATED
execution = WAITING_DELIVERY
last_run_id = 0
completed_runs = 0
```

This state is proven not to have crossed the UART boundary because `main.cpp` commits it before exact spool selection and promotes it to `DELIVERING` before `queueJob()`. It may remain as immutable audit evidence and be superseded by a higher-ID job; it never auto-queues or auto-resumes.

`DELIVERING`, `TIMED_OUT`, accepted/waiting, running and fault states retain fail-closed/manual-review semantics.

For a linked local preparation that stopped before spool selection, `BackupActivityGuard` may consider the machine physically Safe when the live runtime probe is Safe; missing spool provenance in that specific pre-UART state is not promoted into a delivered job. Invalid or mismatched persisted identity still returns Unavailable.

Regression: `Tests/Web/check_job_preparation_transaction.js`.

### B-006 — P2 — network profile recovery could promote uncommitted temp over committed backup — FIXED

Recovery now prefers valid committed main, otherwise valid backup, and promotes temp only when no backup exists (interrupted first write). Invalid evidence fails closed. Regression: `Tests/Web/check_network_profile_atomic_recovery.js`.

## Reviewed without a new production-data defect in this pass

- `PersistentIdAllocator`: verified temp, backup high-water and fail-closed interrupted transaction handling already present.
- `JobSnapshotStore`: immutable create-only final path; no destructive replace of an existing snapshot.
- `JobSpoolSelectionStore`: immutable create-only selection, bounded parse and exact session identity.
- `RepairRegistry`: append-only NDJSON; malformed/partial tail poisons readiness instead of deleting older records.
- `NetworkManager`: AP recovery is started before profile loading.
- `RtcClock`: NTP writes require live runtime Safe and are verified after DS3231 write.
- `WebRecoveryFtpServer`: scoped to `/web`, rechecks live activity and does not write production `/data`.

## Current active target

B-005 is closed at implementation/source-contract level. Continue section B only for concrete findings:

1. remaining backup/restore/activity-guard consistency;
2. remaining mutable persistence stores for destructive swap/ambiguous recovery;
3. resource/NDJSON hotspots only where evidence exists.

If no concrete section-B defect remains, advance to section C desktop/mobile Web/API/error/security parity, then D tests/CI and E documentation/AI routing.

Do not reopen B-001..B-006 without a concrete regression.

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

B-005 preparation boundary smoke when practical:
failed linked preparation before DELIVERING -> no JOB appears on Arduino
next higher-ID job may be created after reboot
DELIVERING/reboot still requires manual review

restore interlock smoke:
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
ESP32 B-001..B-006: FIXED at repo/source-contract level
B-005 exact-current CI: NOT VERIFIED in chat
ESP32 remaining backup/persistence/resource audit: CURRENT
Web audit: NEXT after section B concrete findings exhausted
Tests/CI audit: PENDING
Documentation/AI consistency audit: PENDING
Final repo-wide recheck: PENDING
```
