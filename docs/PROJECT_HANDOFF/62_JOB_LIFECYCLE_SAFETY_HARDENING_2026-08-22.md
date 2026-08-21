# Checkpoint 62 — JOB lifecycle safety hardening

Дата: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

## Scope

Этот checkpoint фиксирует новый production candidate после подтверждённого baseline `e35c4bfe`.

Закрыты три связанных lifecycle edge-case:

1. `TIMED_OUT` больше нельзя закрыть обычным `dismissInactive()`;
2. поздний duplicate `CANCELLED/ALL_CLEAR` после уже завершённого JOB не превращается в storage fault и не переписывает run evidence;
3. immutable `repeat_target` проверяется до append winding event в NDJSON, поэтому завершённая программа не может быть повторно открыта новым `RUN_STARTED`, а `RUN_COMPLETED` выше planned target не попадает в журнал.

## Production commits

```text
ce38711ca9ecd21fa3432e0981f0933878ac85dd
Keep timed-out jobs in manual review

2ecebb5eb7ba41a7dc83209ee4785c77548241fd
Ignore stale cancel after completed job

9e68bd865257ad6f4f070304a38703f4d8aa5445
Reject runs beyond immutable repeat target
```

Regression guards:

```text
fa5a0073ee80c58a1d1cab67d50a1249b9479d00
Guard timed-out job manual review

44d1e03727ab9f95d9cae5e511341ca2e3f89524
Guard stale cancel after completed job

5188084d02f300425f7ce1508f15c968f3ac33f7
Guard immutable repeat target journal boundary
```

## Safety semantics

### TIMED_OUT

Loss of every `JOB_ACK` cannot prove Arduino idle. Therefore:

```text
TIMED_OUT -> ManualReviewRequired
```

Ordinary inactive dismissal permits only proven terminal delivery:

```text
REJECTED
CANCELLED
```

`TIMED_OUT` can be resolved only through the explicit operator manual-review path.

### Late CANCELLED / ALL_CLEAR

A late duplicate cancel acknowledgement after:

```text
ProgramCompleted
ClosedAfterReview
```

is stale transport evidence. `closeAfterRemoteCancel()` returns idempotent success without mutating persisted run evidence.

Running/inter-repeat states still reject cancellation closure unless there is zero physical-run evidence.

### Final repeat boundary

`CM_WindingJournalSnapshotContext.cpp` now loads:

```text
immutable JobSnapshot.repeatTarget
per-session JobRuntimeState
```

before journal append.

Rejected before append:

```text
RUN_STARTED when execution_state == PROGRAM_COMPLETED
RUN_STARTED when completed_runs >= repeat_target
RUN_COMPLETED when event.completed_runs > repeat_target
snapshot/state identity mismatch
```

The guard uses the small session state file rather than adding another full NDJSON scan.

## Regression audit

`Tests/Web/check_job_cancel_recovery_contracts.js` now protects:

- lost ACK -> remote `JOB_CANCEL`;
- idempotent `ALREADY_CLEAR`;
- physical `D -> * -> # -> D` / `ALL_CLEAR`;
- active-run clear rejection;
- no-run persisted cancellation closure;
- stale terminal cancel no-op;
- timeout manual-review isolation;
- immutable repeat-target journal boundary;
- recovery re-evaluation before new JOB creation.

The audit is executed by `.github/workflows/cmp-protocol-tests.yml` with `if: always()` like the other contract audits.

## Verification status

Last exact verified production baseline remains:

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

For this checkpoint's production candidate, exact workflow run IDs have not yet been inspected in this chat after the final changes above.

Therefore current labels are:

```text
ESP32 Build — NOT VERIFIED for current candidate
CMP Protocol Tests — NOT VERIFIED for current candidate
hardware UART/cancel/repeat smoke — NOT VERIFIED for current candidate
```

Do not promote these to GREEN until actual runs are confirmed.

## Next

1. Confirm current-candidate `ESP32 Build` and `CMP Protocol Tests`.
2. If green, perform targeted two-board UART smoke when hardware is available:
   - normal JOB -> physical START -> RUN_STARTED/RUN_COMPLETED;
   - lost/no ACK -> manual review semantics;
   - no-run cancel / ALREADY_CLEAR / ALL_CLEAR;
   - final repeat cannot reopen automatically.
3. After that, select further work only from a concrete defect, measured hotspot, or requested feature.

Historical checkpoints remain evidence only and must not override `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
