# Runtime provenance audit checkpoint — 2026-08-22

Branch: `cmp-protocol-v1`

This checkpoint records the current source-level audit of the ESP32 job-event, winding journal, and manual wire write-off path. It is history/evidence; the active queue remains `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

## Verification boundary

The user explicitly confirmed the pre-audit state GREEN earlier in this session. Changes listed below occurred after that confirmation and must not be called GREEN until fresh applicable CI/operator evidence is supplied.

## Late RUN_STARTED after delivery timeout

A real recovery defect was found in the ESP32 job-state path: a CRC-valid late `RUN_STARTED` for the exact immutable session could be journaled after a lost `JOB_ACK`, but the normal runtime-state transition could still reject the persisted `TIMED_OUT + WAITING_DELIVERY` state.

Fix chain:

```text
57c7b8c...
  route the narrow RUN_STARTED-after-timeout reconciliation through
  JobStateStore::confirmStartedAfterDeliveryTimeout()

fe9fafa...
  add regression contract for late timeout recovery

d2fc3a6...
  run the contract in CMP Protocol Tests
```

Safety boundary remains unchanged:

- timeout alone never proves Arduino idle;
- no automatic START or auto-resume is introduced;
- only a valid `RUN_STARTED` for the exact immutable session is treated as stronger physical-run evidence.

## CMP1 / winding journal audit

`Shared/CMP1Text/CM_Cmp1Crc.h`, Arduino `CM_UartEventTransport.*`, and ESP32 `CM_UartEventReceiver.*` were checked together. CRC-16/MODBUS and current JOB/reply CRC negotiation remain aligned, including legacy ACK compatibility.

`WindingJournal::save(event)` is implemented in `CM_WindingJournalSnapshotContext.cpp`; it loads immutable snapshot/state by exact `event.sessionId`, resolves explicit `WindingEventContext`, and then calls the journal append implementation.

The main journal transition path remains fail-closed:

- exact duplicate event semantics are idempotent;
- `RUN_STARTED` requires no active run and strictly increasing `run_id`;
- `RUN_COMPLETED` requires the exact active run and `completed_runs == previous + 1`;
- transition audit rebuilds ordering from durable NDJSON after reboot;
- completion evidence can be queried by exact `session_id + run_id`.

## Manual wire write-off audit

Both legacy spool and KG_FIRST write-off paths remain explicit/manual and require exact non-zero `source_session_id + source_run_id`. `RUN_COMPLETED` itself does not deduct material.

Duplicate protection uses the authoritative warehouse movement integrity audit and exact source-run lookup. Crash recovery remains fail-closed:

- dangling spool-backed `PENDING` + unchanged pre-write weight => `ABORTED`;
- dangling spool-backed `PENDING` + exact post-write weight => `CONFIRMED`;
- any third/ambiguous weight blocks recovery;
- unallocated historical KG_FIRST `PENDING` has no stock mutation and is closed as `ABORTED` after reboot.

## KG_FIRST exact-spool provenance defect — fixed

A production provenance gap was found.

Current linked JOB creation requires an exact `spool_id` and persists `JobSpoolSelection` before the `DELIVERING` state and before the UART boundary. However `confirmKgFirstWriteOff()` previously accepted `operation.spoolId == 0`, so a write-off request could hide the already immutable selected spool and be recorded as `UNALLOCATED`.

This violated the rule that an exact selected spool must retain exact `spool_id` provenance.

Fix:

```text
cfb5072b16cecbf9016c118391083a1415ce8b71
  Preserve exact spool provenance in KG-first writeoff
```

`WarehouseStore::confirmKgFirstWriteOff()` now rejects both a missing spool id and any spool id different from the immutable session selection for the current linked production path.

Regression:

```text
acbe9cc195eb79b7efa131cb6c23c1325e86f14a
  Guard KG-first exact spool provenance
```

`Tests/Web/check_kg_first_material_contracts.js` now requires the exact-spool guard.

Important compatibility note: the movement schema and recovery code still understand historical `UNALLOCATED` KG_FIRST records. The current linked production preparation path, however, always has an immutable exact spool, so new write-offs for those sessions must not downgrade provenance to unallocated.

## Current next review

Continue with:

1. `CM_JobSnapshotStore.*` temp/orphan recovery and immutable identity behavior;
2. `CM_JobSpoolSelectionStore.*` recovery/identity behavior;
3. job recovery after reboot across snapshot + state + selection;
4. exact run/material finalization coverage;
5. fresh applicable ESP32 Build + CMP Protocol Tests before declaring this post-GREEN batch verified.
