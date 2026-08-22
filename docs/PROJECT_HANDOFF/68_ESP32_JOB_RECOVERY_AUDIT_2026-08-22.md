# ESP32 job recovery audit — 2026-08-22

Branch: `cmp-protocol-v1`

## Baseline

The operator/user explicitly confirmed the preceding Arduino/Core batch GREEN at:

```text
b358f308e032bf484ebfc37b17d29334216e8424
Record Core state-machine audit findings
USER CONFIRMED GREEN
```

The CMP1 text/UART documentation checkpoint after that baseline is `825d84d7a1dcc0019a5bb540361f6218b24e8d48`; this and the recovery commits below require their own CI/user confirmation before they are called GREEN.

## Confirmed defect: late RUN_STARTED after lost JOB_ACK

The persistent store already contained the narrow recovery helper:

```text
JobStateStore::confirmStartedAfterDeliveryTimeout(...)
```

Its contract is correct: only `TIMED_OUT + WAITING_DELIVERY + last_run_id=0 + completed_runs=0` may be reconciled by later physical `RUN_STARTED` evidence. It records `ACCEPTED + RUNNING` for that exact session/run and does not queue or start hardware.

However, production `persistEventState()` routes `RUN_STARTED` through `JobStateStore::updateExecution(...RUNNING...)`. Before this audit, `updateExecution()` applied the normal execution-transition validation first. `WAITING_DELIVERY -> RUNNING` was therefore rejected before the dedicated timeout-reconciliation helper could ever run. The helper existed but was effectively unreachable from the production event path.

Runtime consequence before the fix:

```text
JOB delivery -> ACKs lost -> persistent TIMED_OUT / manual review
Arduino had actually accepted job -> physical START
late/retried RUN_STARTED reaches ESP32
journal accepts physical evidence
state update rejects TIMED_OUT -> RUNNING
ESP32 returns STATE_WRITE_FAILED/NACK
```

This contradicted the intended rule that valid physical evidence is stronger than a lost transport ACK.

## Fix

Commit:

```text
57c7b8cc5ad46442296c524d20049ee69cca8909
Reconcile late RUN_STARTED after delivery timeout
```

`JobStateStore::updateExecution()` now detects only the narrow no-run timeout state before normal execution-transition validation and delegates to `confirmStartedAfterDeliveryTimeout()`.

Guards retained:

- `runId` must be non-zero;
- `RUN_STARTED` completion evidence must remain zero;
- only persisted `TIMED_OUT` is eligible;
- execution must still be `WAITING_DELIVERY`;
- no previous run/completion evidence may exist;
- no queue, resume, SSR, or physical START action is performed by recovery.

## Regression protection

```text
fe9fafa2e4db8d2afb569c37845aa781932b3bf0
Add late RUN_STARTED recovery contract

d2fc3a67a99f1d214801b247beb75208e52c03e6
Run late RUN_STARTED recovery contract
```

`Tests/Protocol/check_job_state_late_run_recovery_contract.js` verifies that:

- the dedicated helper remains limited to timeout/no-run evidence;
- `updateExecution(...RUNNING...)` actually routes the timeout case to that helper;
- completion evidence must be zero for the reconciliation;
- production `main.cpp` continues routing RUN_STARTED persistence through `updateExecution()`;
- recovery does not queue or physically start work.

## Other recovery policy checked in this pass

No additional confirmed defect was found in the inspected cancel/reboot closure paths:

- `TIMED_OUT` remains excluded from ordinary `dismissInactive()` because lost ACK never proves Arduino idle;
- explicit manual review may close ambiguous `DELIVERING`, `TIMED_OUT`, `RUNNING`, `FAULT`, or accepted/no-start state without starting/resuming anything;
- positive Arduino `CANCELLED/ALL_CLEAR` closes only no-run state; existing run/fault evidence remains unchanged and fail-closed;
- reboot recovery sets both `mayAutoQueue=false` and `mayAutoResume=false`;
- immutable snapshot identity is required before persisted runtime state is trusted.

## Next audit owner

Continue with winding journal transition ownership and exact `session_id + run_id` persistence/ACK ordering. Verify that duplicate/late events cannot mutate a different session and that ACK is emitted only after durable journal + runtime state evidence.
