# Checkpoint 53 — write-off hardware fault injection

Date: 2026-08-21
Branch: `cmp-protocol-v1`

This checkpoint defines the next real-device acceptance pass for the kg-first manual wire write-off flow. It does not authorize any automatic physical action or automatic material deduction.

## Safety boundary

The following invariants remain mandatory:

- physical START is physical only;
- ESP32/Web never drives SSR directly;
- no auto-resume after reboot;
- `RUN_COMPLETED` is eligibility evidence only and never writes off wire by itself;
- every new write-off remains manual and tied to exact `source_session_id + source_run_id`;
- legacy exact-spool history remains readable and immutable.

## Code state before hardware test

The backend currently supports:

- `KG_FIRST + SPOOL`: manual quantity in kg, exact immutable spool, guarded stock mutation;
- `KG_FIRST + UNALLOCATED`: manual quantity in kg, conductor snapshot, no spool mutation;
- legacy exact-spool POST remains compatible;
- every POST failure explicitly returns `write_performed:false`;
- startup recovery resolves a dangling write-off before warehouse readiness is exposed.

Relevant fault-contract commits:

```text
d7441a8d  add write-off fault contract test
bdd76ffa  run fault contract test in CI
edd92a15  align fault test with current kg-first UI
875c3e66  normalize POST writeoff failure semantics
f924bb58  enforce explicit write_performed:false failures
```

Build/CI for the current HEAD is not considered confirmed until an actual result is observed.

## Hardware preparation

Use a disposable/test repair and test spool. Do not perform these tests on a production repair that must remain historically clean.

Before each fault case record:

```text
repair_id
source_session_id
source_run_id
spool_id if used
current spool weight
/data/warehouse/movements.ndjson size
/data/warehouse/spools.ndjson size
```

Use the current desktop/mobile write-off page and current ESP32 firmware/web files from `cmp-protocol-v1`.

## Case A — duplicate exact-run write-off

1. Complete one linked winding run normally so `RUN_COMPLETED` exists.
2. Perform one manual kg-first write-off for that exact session/run.
3. Attempt a second POST for the same `source_session_id + source_run_id`.

Expected:

```text
HTTP 409
error = source_run_already_written_off
write_performed = false
no second CONFIRMED movement
no second stock mutation
```

## Case B — CLOSED repair

1. Use a repair that is already `CLOSED` and has readable history.
2. Attempt a new manual write-off.

Expected:

```text
HTTP 409
error = repair_closed
write_performed = false
movement journal unchanged
spool file unchanged
```

## Case C — microSD unavailable before write

1. Power off safely.
2. Remove or otherwise make the microSD unavailable.
3. Boot ESP32.
4. Attempt to open/use manual wire write-off.

Expected:

```text
warehouse/store not ready
POST rejected with warehouse/storage failure
write_performed = false where POST reaches the handler
no material deduction
no physical action
```

Restore the card before continuing. Do not hot-remove the card while a write is actively in progress unless executing a specifically controlled power-cut case below.

## Case D — reboot after KG_FIRST UNALLOCATED PENDING

This case requires a controlled test build or deliberate fault-injection hook that stops execution immediately after the PENDING journal append and before CONFIRMED append. Do not emulate this by manually editing production files.

After reboot, expected startup recovery:

```text
same movement_id gets ABORTED
never CONFIRMED
no spool mutation exists
warehouse becomes ready only after reconciliation succeeds
```

A subsequent manual retry for the same exact run is permitted only if no CONFIRMED write-off exists and all normal completion/provenance checks pass.

## Case E — reboot after KG_FIRST SPOOL PENDING, before stock mutation

Controlled interruption point:

```text
PENDING durable
spool weight still == weight_before_g
```

After reboot expected:

```text
movement closes ABORTED
spool weight remains weight_before_g
no CONFIRMED deduction is invented
```

## Case F — reboot after stock mutation, before CONFIRMED append

Controlled interruption point:

```text
PENDING durable
spool weight already == weight_after_g
CONFIRMED line not yet durable
```

After reboot expected:

```text
movement closes CONFIRMED
same movement_id
same exact source session/run
same conductor snapshot
no second stock mutation
```

This is the only reboot case where recovery may confirm the transaction because durable spool state proves the mutation already happened.

## Case G — ambiguous spool state

Controlled interruption/corruption test only on disposable data:

```text
PENDING exists
current spool weight != expected before
current spool weight != expected after
```

Expected:

```text
recoverPendingWriteOff() fails
WarehouseStore remains not ready
new write-offs are blocked
no guessed CONFIRMED or ABORTED result
```

Do not manually “repair” the journal by deleting lines. Preserve the test media/image for diagnosis.

## Case H — damaged winding completion evidence

On disposable test media only, make the winding journal unreadable/integrity-invalid after creating the test repair/run and before write-off.

Expected POST behavior:

```text
winding_history_unavailable
or winding_history_integrity_failed
write_performed = false
no warehouse mutation
```

## Acceptance evidence to capture

For each case keep:

- HTTP status and JSON body;
- relevant UI message;
- before/after spool weight where applicable;
- final PENDING/CONFIRMED/ABORTED pair for the test movement;
- `/api/system/storage` output including current NDJSON sizes;
- reboot result and whether warehouse endpoints became available;
- serial log around recovery if available.

## Pass criteria

The hardware fault block is accepted only if all tested failure paths are deterministic and fail closed:

```text
no automatic physical START
no auto-resume
no RUN_COMPLETED-triggered deduction
no duplicate exact-run write-off
no guessed recovery result
no hidden stock mutation on rejected POST
```

## Next repo work after hardware evidence

Use measured file sizes/latencies from the device before changing NDJSON rotation or segmentation. Do not introduce destructive compaction, arbitrary rotation thresholds, or a database migration without measured need and a provenance-preserving migration design.
