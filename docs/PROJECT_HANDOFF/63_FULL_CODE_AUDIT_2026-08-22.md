# Checkpoint 63 — Full code audit phase

Date: 2026-08-22  
Branch: `cmp-protocol-v1`

## Purpose

This checkpoint owns the active full-code audit. Phase 9 Web implementation is complete in checkpoint 64. The targeted JOB/UART desync review is summarized in checkpoint 65. ESP32 build-identity CI recovery is recorded in checkpoint 66.

Historical checkpoints are evidence, not an automatic task queue.

## Verification baseline

The operator explicitly reported **all visible workflows green** immediately before the B-002 implementation in this continuation. Treat that as **USER CONFIRMED GREEN** for the then-current branch state only; it does not prove commits created after that statement.

Current B-002 implementation/test commits therefore remain **exact-current verification pending** until a matching ESP32 Build and CMP Protocol Tests result is inspected or explicitly confirmed.

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

### A-001 — P1 — remote JOB could overwrite non-idle Arduino state — FIXED

Remote job/session IDs remain non-zero and exact; only truly empty HOME may accept a new identity; exact zero-run lost-ACK duplicate is idempotent.

### A-002 — P2 — permissive Arduino JOB parsing — FIXED

Strict full-token bounded decimal parsing and exact `STARTING` / `WORKING` validation are implemented. Regression coverage is present in Protocol CI.

### A-003 — P2 — permissive ACK/NACK and JOB_CANCEL IDs — FIXED

Inbound Arduino correlation IDs require complete canonical decimal tokens; numeric prefixes/trailing garbage cannot acknowledge/cancel a different event.

### A-004 — P1 — stale zero-id ALL_CLEAR could correlate to a fresh JOB — FIXED

Zero-id `ALL_CLEAR` correlates only to explicit pending cancel or a dedicated persisted-recovery identity. Fresh `queueJob()` and successful physical RUN evidence disarm that recovery identity.

### A-005 — P1 — stale cancel against run/fault evidence could falsely mark storage failed — FIXED

`closeAfterRemoteCancel()` treats unsafe run/fault evidence as unchanged no-op, preserving manual review and storage availability. Only zero-run waiting state is rewritten as cancelled.

### A-006 — P1 — control reply/timeout could be persisted after following RUN evidence — FIXED

Receiver ordering barriers force pending JOB/CANCEL control results to be drained/persisted before a later physical RUN frame is parsed.

### A-007 — P1 — lost JOB_ACK timeout plus real RUN_STARTED was unrecoverable — FIXED

A narrow retry-safe state transition reconciles only exact `TIMED_OUT + WAITING_DELIVERY + zero-run` with a later CRC-valid `RUN_STARTED` for the same immutable session. No physical action is introduced. RAM status and recovery-only ALL_CLEAR identity are normalized after committed RUN evidence.

Full UART details:

```text
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
```

### B-001 — P1 — backup activity guard could promote unknown runtime to Safe — FIXED

`BackupActivityGuard::check()` now requires runtime `Safe` before persisted state/snapshot/spool checks. Runtime `Busy` remains Busy and runtime `Unavailable` remains Unavailable; persisted files can no longer promote an unknown live state to Safe.

Regression protection remains in `Tests/Web/check_final_acceptance_contracts.js`.

### B-002 — P1 — restore/apply lacked a global production-mutation interlock — FIXED / exact-current verification pending

Concrete race was that restore apply spans many loop iterations while ordinary HTTP mutation routes were still dispatchable before the next restore update.

Implemented cross-layer fix:

```text
29ef6600c4b9c078826abbbc49c398a4326a9560
Add restore production mutation interlock

1c1272ec90092767c8909632af79a099539e553a
Interlock restore production mutations

b8f7bb93a9b5861bc584a648b1605b05bce5776f
Guard restore production mutation interlock

c953665077c810455ccbf31d6ea260a8aaed2e3c
Run restore mutation interlock audit
```

Current contract:

1. `CM_ProductionMutationInterlock.h` owns one process-wide restore production-mutation lock.
2. `ProductionMutationInterlockHandler` is registered during global `RemoteBackupWeb` construction, before `configureWebServer()` later registers application routes.
3. While the lock is active, every non-GET `/api/*` request is intercepted with HTTP `409` and `restore_mutation_active`; GET/status/read-only routes remain dispatchable.
4. `RemoteBackupWeb::ApplyStageState` acquires/retains the lock for forward APPLY, COPY, VERIFY, rollback states, APPLIED and FAILED.
5. `RolledBack` and explicit `Idle` cleanup release it.
6. APPLIED deliberately stays locked until reboot because in-memory stores were loaded from pre-restore production files; allowing writes before reboot would reintroduce stale-RAM corruption risk.
7. FAILED also stays fail-closed until reboot/cleanup.
8. Existing restore owner already calls `BackupActivityGuard::check()` in forward apply entry/copy/verification paths. Losing proven Safe stops the forward step and `update()` enters the existing rollback path.
9. Operator-only exact `confirmed=APPLY`, `auto_resume=0`, physical START ownership and SSR ownership are unchanged.

Regression guard:

```text
Tests/Web/check_restore_mutation_interlock.js
```

The handler signature is intentionally compatible with the project's pinned Arduino-ESP32 `2.0.17` `RequestHandler` API (`HTTPMethod, String`).

### B-003 — P2 — network profile API conflated storage failures with client validation — FIXED / exact-current verification pending

Implemented before B-002 completion:

```text
1048dd922d059b3f8ec0f31fafc3bd24795688af
5c05ad2f636ff5548ca80317010346dd66ad1af5
```

Current Network API semantics distinguish:

```text
400 invalid/missing operator input
404 requested profile absent
409 profile capacity/conflict
500 persistence/write/remove/reload failure with ready storage
503 profile store/manager unavailable
```

AP recovery behavior and profile schema remain unchanged.

## Current active target

Continue section B with the first still-open review block:

1. persistence/atomic-write/integrity partial-failure paths;
2. network/AP/STA/FTP/RTC/SD fail-closed behavior;
3. remaining backup/restore/activity-guard consistency;
4. resource/NDJSON hotspots only where evidence exists.

Do not reopen B-001, B-002 or B-003 without a concrete regression.

Then continue sections C, D, E and final cross-layer recheck.

## External hardware verification gate

Still required when the stand is available:

```text
normal JOB -> Arduino READY
physical START only
RUN_STARTED -> RUN_COMPLETED
repeat > 1 -> physical START each run
no automatic material writeoff

zero-run cancel
ALREADY_CLEAR
safe physical ALL_CLEAR
late zero-id ALL_CLEAR must not cancel a fresh job
lost JOB_ACK -> TIMED_OUT/manual review -> late RUN_STARTED reconciliation
reboot in waiting/running states -> no auto resume

restore apply interlock smoke when practical:
GET status remains available during APPLY
POST/DELETE API mutation receives 409 restore_mutation_active
APPLIED requires reboot before mutations resume
```

## Execution rules

For every existing file changed:

1. fetch current `cmp-protocol-v1` content and blob SHA;
2. make the smallest safe fix;
3. add/extend regression coverage where practical;
4. update current docs after semantics change;
5. never claim GREEN until the named workflow passes on the exact candidate or the operator explicitly confirms that state.

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
ESP32 persistence/atomic/integrity audit: CURRENT
ESP32 network/FTP/RTC/SD review: PENDING
Remaining backup/restore review: PENDING
Web audit: PENDING
Tests/CI audit: PENDING
Documentation/AI consistency audit: PENDING
Final repo-wide recheck: PENDING
Current post-B-002 candidate CI: NOT VERIFIED in chat
```
