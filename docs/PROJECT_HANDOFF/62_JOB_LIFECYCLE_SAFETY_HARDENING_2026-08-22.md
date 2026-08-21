# Checkpoint 62 — JOB lifecycle safety hardening

Дата: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

## Scope

Этот checkpoint фиксирует verified production baseline после lifecycle/integrity hardening.

Закрыты четыре связанных lifecycle/integrity edge-case:

1. `TIMED_OUT` больше нельзя закрыть обычным `dismissInactive()`;
2. поздний duplicate `CANCELLED/ALL_CLEAR` после уже завершённого JOB не превращается в storage fault и не переписывает run evidence;
3. immutable `repeat_target` проверяется до append winding event в NDJSON, поэтому завершённая программа не может быть повторно открыта новым `RUN_STARTED`, а `RUN_COMPLETED` выше planned target не попадает в журнал;
4. deep session persistence audit cross-check-ит persisted `JobRuntimeState` против immutable snapshot repeat target, поэтому уже сохранённый state с impossible completed count fail-closed блокирует backup/integrity acceptance.

## Production commits

```text
ce38711ca9ecd21fa3432e0981f0933878ac85dd
Keep timed-out jobs in manual review

2ecebb5eb7ba41a7dc83209ee4785c77548241fd
Ignore stale cancel after completed job

9e68bd865257ad6f4f070304a38703f4d8aa5445
Reject runs beyond immutable repeat target

5fa6bcea812c33f0b2dc8e13baae476221839b3a
Validate session state against repeat target
```

Regression guards:

```text
fa5a0073ee80c58a1d1cab67d50a1249b9479d00
Guard timed-out job manual review

44d1e03727ab9f95d9cae5e511341ca2e3f89524
Guard stale cancel after completed job

5188084d02f300425f7ce1508f15c968f3ac33f7
Guard immutable repeat target journal boundary

5e407f2e57946be31b7c740bd4b40fea363b0d59
Guard repeat target session integrity
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

`CM_WindingJournalSnapshotContext.cpp` loads:

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

### Deep session integrity

`CM_WindingSessionPersistenceIntegrityAudit.cpp` validates snapshot/state semantics in the same per-session pass already used for identity checks:

```text
state.completed_runs <= snapshot.repeat_target
PROGRAM_COMPLETED -> completed_runs == repeat_target
WAITING_PHYSICAL_START/RUNNING -> completed_runs < repeat_target
```

`ClosedAfterReview` and `Fault` may preserve partial/final evidence, but can never exceed the immutable target.

This adds no `WindingJournalQuery` pass and does not reintroduce duplicate full journal scanning.

## Regression audit

`Tests/Web/check_job_cancel_recovery_contracts.js` protects:

- lost ACK -> remote `JOB_CANCEL`;
- idempotent `ALREADY_CLEAR`;
- physical `D -> * -> # -> D` / `ALL_CLEAR`;
- active-run clear rejection;
- no-run persisted cancellation closure;
- stale terminal cancel no-op;
- timeout manual-review isolation;
- immutable repeat-target journal boundary;
- snapshot/state repeat-target integrity;
- no extra full journal scan for those guards;
- recovery re-evaluation before new JOB creation.

The audit is executed by `.github/workflows/cmp-protocol-tests.yml` with `if: always()` like the other contract audits.

## Verified baseline

ESP32 production C++ changed through:

```text
5fa6bcea812c33f0b2dc8e13baae476221839b3a
Validate session state against repeat target
```

Verified exact Actions result:

```text
ESP32 Build #1245 — GREEN
run 32515224487
head_sha 5fa6bcea812c33f0b2dc8e13baae476221839b3a
```

After `5fa6bcea`, the commits through `ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133` changed only regression tests and documentation, not ESP32 production C++.

Current lifecycle regression suite is verified on:

```text
ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133
CMP Protocol Tests #2210 — GREEN
run 32515361340
```

The immediately preceding docs state was also green:

```text
5f6272f51ba3c81d94bf24894a08667219d91e8d
CMP Protocol Tests #2209 — GREEN
run 32515329411
```

Therefore checkpoint 62 repo-level status is:

```text
ESP32 production build — GREEN
CMP Protocol Tests / lifecycle audit — GREEN
hardware UART/cancel/repeat smoke — NOT VERIFIED for checkpoint 62
```

## Next

1. Do not reopen checkpoint-62 lifecycle work without a concrete regression.
2. When hardware is available, perform targeted two-board UART smoke:
   - normal JOB -> physical START -> RUN_STARTED/RUN_COMPLETED;
   - lost/no ACK -> manual review semantics;
   - no-run cancel / ALREADY_CLEAR / ALL_CLEAR;
   - final repeat cannot reopen automatically;
   - backup/session audit remains stable on normal persisted sessions.
3. Otherwise continue only from a concrete bug, measured hotspot, or requested feature.

Historical checkpoints remain evidence only and must not override `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
