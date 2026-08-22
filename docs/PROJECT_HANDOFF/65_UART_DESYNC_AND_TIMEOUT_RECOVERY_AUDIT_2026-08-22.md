# Checkpoint 65 — UART desync and timeout recovery audit

Date: 2026-08-22  
Branch: `cmp-protocol-v1`

## Status

Targeted ESP32<->Arduino JOB/UART desync audit has produced concrete fixes. Generic JOB cancellation was not reimplemented; only proven correlation, ordering and recovery defects were changed.

Current implementation HEAD when this checkpoint was written:

```text
bf3ac8c18a3d8b484eaf755452965b670973f627
Guard timeout RUN reconciliation contracts
```

Verification for that exact HEAD:

```text
CMP Protocol Tests: NOT VERIFIED
ESP32 Build: NOT VERIFIED
Arduino Uno Build: NOT VERIFIED
hardware two-board smoke: NOT VERIFIED
```

Empty combined-status / commit-workflow results are not proof that Actions passed.

## Closed findings

### A-002 — strict Arduino JOB parser — FIXED / exact-current CI pending

Production JOB parsing now rejects malformed-but-CRC-valid decimal/type tokens fail-closed. Current ESP32 sender already emits canonical decimal fields and exact `STARTING` / `WORKING` tokens.

### A-003 — strict ACK/NACK and JOB_CANCEL correlation parsing — FIXED / exact-current CI pending

Arduino no longer accepts numeric prefixes/trailing garbage for `ACK/NACK run_id` or `JOB_CANCEL job_id`.

### A-004 — zero-id ALL_CLEAR could cancel a fresh JOB — FIXED / exact-current CI pending

Previous ESP32 logic could correlate:

```text
CMP1|JOB_CANCEL_ACK|0|CANCELLED|ALL_CLEAR|...
```

against a new pending or last queued JOB. A late identity-less physical clear could therefore close the wrong fresh zero-run job.

Current rule:

- exact pending `JOB_CANCEL` identity remains authoritative;
- zero-id `ALL_CLEAR` may use only an explicit pending cancel or a dedicated persisted-recovery identity;
- `queueJob()` clears that recovery-only identity before sending any fresh JOB;
- successful physical RUN evidence clears the recovery-only identity again;
- zero-id `ALL_CLEAR` never infers identity from `m_pendingJob` or `m_lastQueuedJobId`.

Relevant implementation:

```text
firmware/esp32/src/CM_UartEventReceiver.h
firmware/esp32/src/CM_UartEventReceiver.cpp
Tests/Web/check_job_cancel_recovery_contracts.js
```

### A-005 — stale cancel/run evidence could create false storage failure — FIXED / exact-current CI pending

A late `CANCELLED/ALL_CLEAR` against persisted `RUNNING`, `FAULT` or other run evidence previously made `closeAfterRemoteCancel()` return `false`; caller then marked `jobStateStoreReady=false`, which could prevent the operator manual-review path even though storage itself was healthy.

Current rule in `CM_JobStateRemoteCancel.cpp`:

- real load/write failure still returns `false`;
- completed/operator-closed state is an idempotent no-op;
- any run/fault evidence is also a no-op and is never rewritten as cancellation;
- only zero-run `WaitingDelivery` / `WaitingPhysicalStart` is closed as cancelled;
- unchanged unsafe state continues to require explicit manual review.

### A-006 — JOB reply/timeout could be processed after following RUN evidence — FIXED / exact-current CI pending

Two ordering races existed:

1. `poll()` could parse `JOB_ACK` and then return a following `RUN_STARTED` in the same call before `processJobDelivery()` persisted `ACCEPTED`.
2. `receiver.update()` could publish `TIMED_OUT`, then `poll()` could consume `RUN_STARTED` before that timeout event was persisted.

Current transport ordering:

- pending `JobDeliveryEvent` or `JobCancelEvent` blocks further RUN parsing until main drains it;
- after parsing `JOB_ACK` / `JOB_CANCEL_ACK`, `poll()` returns through an ordering barrier;
- main persists delivery/cancel state before the next physical RUN frame is consumed.

### A-007 — lost JOB_ACK + TIMED_OUT + real RUN_STARTED was unrecoverable — FIXED / exact-current CI pending

If every JOB_ACK was lost, ESP32 persisted `TIMED_OUT`. A later CRC-valid `RUN_STARTED` for the exact immutable session proves Arduino did receive the JOB and a physical START happened, but the old runtime transition rejected it and could produce `STATE_WRITE_FAILED`.

Current narrow recovery:

```text
firmware/esp32/src/CM_JobStateLateRunRecovery.cpp
```

`confirmStartedAfterDeliveryTimeout()` accepts only:

```text
delivery_state = TIMED_OUT
execution_state = WAITING_DELIVERY
last_run_id = 0
completed_runs = 0
exact session_id
non-zero run_id
```

and atomically changes persisted state to:

```text
delivery_state = ACCEPTED
execution_state = RUNNING
last_run_id = exact RUN_STARTED run_id
completed_runs = 0
```

No motor action occurs in this transition.

`CM_WindingJournalSnapshotContext.cpp` keeps this retry-safe:

1. validate immutable snapshot/runtime identity and repeat target;
2. append/recognize the physical event as `Saved` or `Duplicate`;
3. perform the narrow runtime reconciliation;
4. if the state write fails, Arduino retry sees `Duplicate` and can retry reconciliation without inventing another physical RUN.

After successful remote RUN evidence, `main.cpp` also normalizes RAM:

```text
lastJobResult = Accepted
clear recovery-only ALL_CLEAR identity
```

This prevents a non-final recovered repeat from being displayed as `TIMED_OUT` and keeps backup activity fail-closed while more repeats remain.

## Preserved safety semantics

None of the fixes change physical ownership:

```text
physical START remains Arduino/local only
no automatic START between repeats
no auto-resume after reboot
ESP32/Web do not control SSR
RUN_COMPLETED does not auto-write off wire/material
manual writeoff keeps exact source_session_id + source_run_id
zero-id ALL_CLEAR never fabricates RUN completion
unresolved TIMED_OUT remains manual-review required
RUNNING/FAULT evidence is never erased by cancel recovery
```

## Regression protection

`Tests/Web/check_job_cancel_recovery_contracts.js` now checks:

- lost-ACK remote cancel handoff;
- control-result-before-RUN ordering barriers;
- narrow timeout-to-RUN recovery;
- journal-first retry semantics;
- runtime normalization after recovered RUN evidence;
- recovery-only zero-id ALL_CLEAR identity;
- stale/run-evidence cancel no-op;
- unresolved timeout manual-review isolation;
- immutable repeat-target guards;
- Arduino physical emergency-clear safety.

## Next active audit work

The targeted UART code review is now sufficiently closed at repo level to move forward. Do not automatically return to generic JOB cancel/recovery unless a new concrete regression is observed.

Continue checkpoint 63 with:

1. ESP32 runtime/API/persistence/integrity audit;
2. network/FTP/RTC/SD failure semantics;
3. backup/restore/activity-guard consistency;
4. desktop/mobile Web/API/error/security parity;
5. tests/CI/build trigger/filter audit;
6. documentation/AI routing consistency;
7. final cross-layer recheck and exact applicable CI verification.

Hardware validation remains an external required gate, especially:

```text
normal JOB -> READY -> physical START -> RUN_STARTED -> RUN_COMPLETED
lost JOB_ACK -> timeout/manual review -> late valid RUN_STARTED reconciliation
late zero-id ALL_CLEAR must not cancel a fresh job
cancel of zero-run accepted job
ALREADY_CLEAR
reboot with waiting/running state -> no auto resume
repeat > 1 -> WAITING_NEXT_REPEAT after non-final recovered completion
no automatic material writeoff
```
