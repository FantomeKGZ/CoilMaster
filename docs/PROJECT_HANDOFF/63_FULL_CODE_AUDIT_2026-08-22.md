# Checkpoint 63 — Full code audit phase

Date: 2026-08-22  
Branch: `cmp-protocol-v1`

## Purpose

This checkpoint owns the active full-code audit. Phase 9 Web implementation is complete in checkpoint 64. The targeted JOB/UART desync review has now produced concrete fixes and is summarized in checkpoint 65.

Historical checkpoints are evidence, not an automatic task queue.

## Verified baseline entering the audit

Previously verified automated gates:

```text
ESP32 Build #1245 — GREEN
run 32515224487
head_sha 5fa6bcea812c33f0b2dc8e13baae476221839b3a

CMP Protocol Tests #2210 — GREEN
run 32515361340
head_sha ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133
```

These runs do not prove newer audit commits green.

Current UART audit implementation checkpoint:

```text
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
```

Exact current audit-head CI remains **NOT VERIFIED** until matching workflow runs are inspected.

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

### A-001 — P1 — remote JOB could overwrite non-idle Arduino state — FIXED / exact-current verification pending

Fix:

```text
20f422dd8170139ed7caac6159edb23fe7775103
Harden Arduino remote job admission
```

Remote job/session IDs must remain non-zero and exact; only truly empty HOME may accept a new identity; exact zero-run lost-ACK duplicate is idempotent.

### A-002 — P2 — permissive Arduino JOB parsing — FIXED / exact-current verification pending

Strict full-token bounded decimal parsing and exact `STARTING` / `WORKING` validation are implemented. Regression coverage is present in Protocol CI.

### A-003 — P2 — permissive ACK/NACK and JOB_CANCEL IDs — FIXED / exact-current verification pending

Inbound Arduino correlation IDs now require complete canonical decimal tokens; numeric prefixes/trailing garbage cannot acknowledge/cancel a different event.

### A-004 — P1 — stale zero-id ALL_CLEAR could correlate to a fresh JOB — FIXED / exact-current verification pending

Zero-id `ALL_CLEAR` now correlates only to explicit pending cancel or a dedicated persisted-recovery identity. Fresh `queueJob()` and successful physical RUN evidence disarm that recovery identity.

### A-005 — P1 — stale cancel against run/fault evidence could falsely mark storage failed — FIXED / exact-current verification pending

`closeAfterRemoteCancel()` now treats unsafe run/fault evidence as unchanged no-op, preserving manual review and storage availability. Only zero-run waiting state is rewritten as cancelled.

### A-006 — P1 — control reply/timeout could be persisted after following RUN evidence — FIXED / exact-current verification pending

Receiver ordering barriers now force pending JOB/CANCEL control results to be drained/persisted before a later physical RUN frame is parsed.

### A-007 — P1 — lost JOB_ACK timeout plus real RUN_STARTED was unrecoverable — FIXED / exact-current verification pending

A narrow retry-safe state transition now reconciles only exact `TIMED_OUT + WAITING_DELIVERY + zero-run` with a later CRC-valid `RUN_STARTED` for the same immutable session. No physical action is introduced. RAM status and recovery-only ALL_CLEAR identity are normalized after committed RUN evidence.

Full UART details:

```text
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
```

### B-001 — P1 — backup activity guard could promote unknown runtime to Safe — FIXED / exact-current verification pending

Before the fix, `BackupActivityGuard::check()` continued into persisted ESP32 job-state validation when `runtimeCheck()` returned `Unavailable`. If no active persisted job was found, the guard could return `Safe` even though transient RAM-only activity such as local Arduino winding/control state was not provable.

Fix:

```text
5c06538a4781f57a83a1884827b3bc5a3033ef03
Fail closed when backup runtime state unavailable

6528257227d1d6eec7a8e1d8fe5fab7433f0f577
Guard unavailable backup runtime fail-closed
```

Current rule:

```text
runtime Busy        -> Busy
runtime Unavailable -> Unavailable
runtime Safe        -> then persisted state/snapshot/spool identity is also checked
```

Persisted files are now an additional safety check, never a substitute for live runtime proof.

Regression protection is in `Tests/Web/check_final_acceptance_contracts.js`.

### B-002 — P1 — restore/apply lacks a global production-mutation interlock — OPEN

Concrete cross-layer race:

- restore/apply checks `BackupActivityGuard::check()` when the operator starts the operation;
- `RemoteBackupWeb::update()` later continues production-file apply/copy/verification across many loop iterations;
- `webServer.handleClient()` runs before `remoteBackupWeb.update()`;
- ordinary production mutation endpoints, including new JOB creation and other data writes, do not share an explicit `restore mutation active` lock;
- apply-loop itself does not re-check full activity before every production-file mutation chunk.

Therefore an HTTP mutation or newly observed physical/local activity can begin after restore start while working data is being replaced.

Do **not** close this with a JOB-only patch. Required fix must be cross-layer and fail-closed:

1. expose one authoritative production-data mutation lock/state for restore apply + rollback;
2. block normal production mutation endpoints while that lock is active;
3. re-check runtime activity during apply before continuing production-file mutations;
4. if runtime ceases to be proven Safe after apply has started, stop forward apply and enter existing rollback path;
5. keep GET/status/read-only endpoints available;
6. preserve operator-only APPLY confirmation and no-auto-resume semantics.

This remains the first open P1 in section B.

### B-003 — P2 — network profile API conflates storage failures with client validation — UNDER REVIEW

`CM_NetworkWeb.cpp` currently returns `400` for some `NetworkProfileStore` unavailable/write/remove failures. This makes microSD/storage faults indistinguishable from invalid request data and can cause UI to prompt the operator to edit valid input instead of reporting device/storage failure.

Next step: split request validation from store readiness/write outcomes while preserving the existing profile schema and AP recovery behavior.

## Current active target

Continue section B in this order:

1. B-002 global restore/apply production-mutation interlock design/implementation;
2. B-003 network HTTP/storage error semantics;
3. persistence/atomic-write/integrity partial-failure paths;
4. network/AP/STA/FTP/RTC/SD fail-closed behavior;
5. remaining backup/restore/activity-guard consistency;
6. resource/NDJSON hotspots only where evidence exists.

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
```

## Execution rules

For every existing file changed:

1. fetch current `cmp-protocol-v1` content and blob SHA;
2. make the smallest safe fix;
3. add/extend regression coverage where practical;
4. update current docs after semantics change;
5. never claim GREEN until the named workflow passes on the exact SHA.

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
Arduino findings A-001..A-003: FIXED / exact-current verification pending
Targeted UART findings A-004..A-007: FIXED / exact-current verification pending
Targeted UART repo review: COMPLETE -> hardware gate retained
ESP32 B-001: FIXED / exact-current verification pending
ESP32 B-002: OPEN P1 / CURRENT HIGH PRIORITY
ESP32 B-003: UNDER REVIEW P2
ESP32 runtime/API/persistence/integrity/network/backup audit: IN PROGRESS / CURRENT
Web audit: PENDING
Tests/CI audit: PENDING
Documentation/AI consistency audit: PENDING
Final repo-wide recheck: PENDING
Current newer HEAD CI: NOT VERIFIED
```
