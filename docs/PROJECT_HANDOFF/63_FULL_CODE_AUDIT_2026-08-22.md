# Checkpoint 63 — Full code audit phase

Date: 2026-08-22  
Branch: `cmp-protocol-v1`

## Purpose

This checkpoint owns the active full-code audit. Phase 9 Web implementation is complete in checkpoint 64. The targeted JOB/UART desync review is summarized in checkpoint 65. ESP32 build-identity CI recovery is recorded in checkpoint 66.

Historical checkpoints are evidence, not an automatic task queue.

## Verification baseline

The operator explicitly reported **all visible workflows green** immediately before the B-002 implementation in this continuation. Treat that as **USER CONFIRMED GREEN** for the then-current branch state only; it does not prove commits created after that statement.

All B-002 and later implementation/test commits below therefore remain **exact-current verification pending** until matching current workflow results are inspected or explicitly confirmed.

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

`BackupActivityGuard::check()` now requires runtime `Safe` before persisted state/snapshot/spool checks. Runtime `Busy` remains Busy and runtime `Unavailable` remains Unavailable. Regression protection remains in `Tests/Web/check_final_acceptance_contracts.js`.

### B-002 — P1 — restore/apply lacked a global production-mutation interlock — FIXED / exact-current verification pending

Implemented:

```text
29ef6600c4b9c078826abbbc49c398a4326a9560  Add restore production mutation interlock
1c1272ec90092767c8909632af79a099539e553a  Interlock restore production mutations
b8f7bb93a9b5861bc584a648b1605b05bce5776f  Guard restore production mutation interlock
c953665077c810455ccbf31d6ea260a8aaed2e3c  Run restore mutation interlock audit
```

Current contract:

- one process-wide restore production-mutation lock;
- first WebServer handler blocks every non-GET `/api/*` with `409 restore_mutation_active` while APPLY/rollback owns the lock;
- GET/status/read-only routes remain available;
- forward apply still rechecks live `BackupActivityGuard` during entry/copy/verification and enters existing rollback on loss of proven Safe;
- APPLIED/FAILED remain locked until reboot/cleanup because RAM stores still represent pre-restore files;
- RolledBack/Idle release the lock;
- operator-only `confirmed=APPLY`, `auto_resume=0`, physical START and SSR ownership unchanged.

### B-003 — P2 — network profile API conflated storage failures with client validation — FIXED / exact-current verification pending

Implemented:

```text
1048dd922d059b3f8ec0f31fafc3bd24795688af
5c05ad2f636ff5548ca80317010346dd66ad1af5
```

Current semantics distinguish 400 validation, 404 absent profile, 409 capacity/conflict, 500 persistence/reload failure and 503 unavailable store/manager.

### B-004 — P1 — JobStateStore deleted the authoritative state before replacement commit — FIXED / exact-current verification pending

Previous `writeAtomic()` sequence was effectively:

```text
write+verify temp
remove session-N.json
rename temp -> session-N.json
```

A brownout/rename failure between delete and rename could destroy the only persisted delivery/RUN/manual-review state.

Implemented:

```text
02914c2cf68e18497d5fe4f8a4e0a076ad11a7de  Preserve job state across atomic replace
876e8a24903cc6d4e7b75f712054ff39f01df396  Make job state replacement recoverable
8f5482889e9de48f2ea59689541484a2683a43d8  Guard recoverable job state replacement
e27ad41d365b7c4f7efc1266b8f2f484a76945de  Run job state atomic replacement audit
```

Current transaction order:

```text
write + parse/identity verify temp
old target -> .bak
verified temp -> target
re-read + verify committed target
remove .bak only after committed verification
```

If rename/cleanup fails, old state is restored where possible; otherwise `.bak/.tmp` evidence is preserved and the strict state-directory scan fails closed. The code no longer deletes the only authoritative runtime-state before committing its replacement.

Regression: `Tests/Web/check_job_state_atomic_replace.js`.

### B-005 — P2 — linked JOB preparation can leave an orphan spool-selection if runtime-state commit fails — OPEN / design needed

Current linked JOB preparation order is:

```text
allocate IDs
create immutable snapshot
create immutable exact spool selection
create/update runtime state
queue UART JOB
```

If spool selection succeeds but runtime-state creation fails, UART JOB is not queued, but snapshot + selection can remain. Deep session audit rejects a selection without matching runtime state. Blind cleanup is **not** safe: a state write can report failure after a target file has actually appeared, so deleting immutable provenance on the error return could destroy valid evidence.

Do not fix this by simply deleting snapshot/selection on any state error. Required solution must distinguish an uncommitted preparation from a committed state, for example through an explicit preparation/commit marker or an exact post-failure state identity check with narrowly scoped uncommitted cleanup.

### B-006 — P2 — network profile recovery could promote an uncommitted temp over the last committed backup — FIXED / exact-current verification pending

In `NetworkProfileStore::recoverFileSwap()`, the crash state after:

```text
ProfilesPath -> BackupPath
# power loss before TempPath -> ProfilesPath
```

contains valid temp + valid backup but no valid main. The old code preferred temp, making a prepared but not committed network edit authoritative after reboot.

Implemented:

```text
2ab7536db3d2cd6516df7b2cb4688658615110a3  Recover committed network profile state first
9a86b21148a88658b3300d40183384fbc1f15050  Guard committed network profile recovery
e8cdcea4cddf5ce5977229835e31a616695ba51d  Run network profile atomic recovery audit
```

Current recovery order:

```text
valid main -> keep main and clean stale residues
no valid main + valid backup -> restore backup and discard prepared temp
invalid backup evidence -> fail closed
no backup + valid temp -> allow temp promotion only as interrupted first write
otherwise -> fail closed
```

## Reviewed without a new production-data defect in this pass

- `PersistentIdAllocator`: verified temp, backup high-water and fail-closed interrupted transaction handling already present.
- `JobSnapshotStore`: immutable create-only final path; no destructive replace of an existing snapshot.
- `JobSpoolSelectionStore`: immutable create-only selection, bounded parse and exact session identity; begin only promotes a single fully valid temp when final is absent.
- `RepairRegistry`: append-only NDJSON; partial append poisons readiness and reboot integrity audit detects malformed tail without deleting older records.
- `NetworkManager`: AP recovery is started before profile loading; profile-store failure does not remove the AP recovery surface.
- `RtcClock`: NTP writes require live runtime Safe and are verified after DS3231 write.
- `WebRecoveryFtpServer`: FTP is scoped to `/web`, rechecks live activity during operation and stops on Busy/Unavailable; it does not write production `/data`.

## Current active target

Continue section B in this order:

1. resolve B-005 only with a provenance-safe transaction design;
2. remaining backup/restore/activity-guard consistency;
3. remaining mutable persistence stores for destructive swap/partial-failure behavior;
4. resource/NDJSON hotspots only where evidence exists.

If no further concrete section-B defect remains, advance to section C desktop/mobile Web/API parity.

Do not reopen B-001..B-004 or B-006 without a concrete regression.

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
ESP32 B-001: FIXED
ESP32 B-002: FIXED / exact-current verification pending
ESP32 B-003: FIXED / exact-current verification pending
ESP32 B-004: FIXED / exact-current verification pending
ESP32 B-005: OPEN P2 / CURRENT TRANSACTION DESIGN
ESP32 B-006: FIXED / exact-current verification pending
ESP32 persistence/atomic/integrity audit: IN PROGRESS
Remaining backup/restore review: CURRENT AFTER B-005
Web audit: PENDING
Tests/CI audit: PENDING
Documentation/AI consistency audit: PENDING
Final repo-wide recheck: PENDING
Current post-B-006 candidate CI: NOT VERIFIED in chat
```
