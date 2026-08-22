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

Historical `UNALLOCATED` KG_FIRST records remain readable by the movement codec/history/recovery. The current linked production preparation path, however, always has an immutable exact spool, so new write-offs for those sessions must not downgrade provenance to unallocated.

## Linked session selection integrity defect — fixed

The deep winding-session persistence audit previously validated only the forward direction: every existing spool-selection file had to match a linked snapshot/state. It did not require the reverse direction, so deleting the immutable selection file from an already-delivered/running/completed linked session could leave the remaining snapshot/state looking valid to that audit.

A missing selection is legitimate only during the local preparation crash window, because the authoritative preparation order is:

```text
snapshot -> CREATED state -> exact spool selection -> DELIVERING -> UART queue
```

Fix:

```text
02f6e9d432696430f68a9dc4cd1dabe0d4319399
  Require linked spool selection after preparation
```

`CM_WindingSessionPersistenceIntegrityAudit.cpp` now requires a valid exact selection for every linked state that is no longer `JobStateStore::isLocalPreparation(state)`. The selection must match exact `session_id`, `job_id`, `repair_id`, and `motor_id`.

Regression:

```text
a19390ba977047421c4016c1a8369d2f6a07ecdb
  Guard linked selection persistence integrity
```

Startup runtime already has a separate fail-closed linked-selection check after immutable display recovery. If the selection store is unavailable or the recovered selection does not match exact job/session/repair/motor identity, new job creation remains blocked.

## Exact-run finalization coverage defect — fixed

Historic legacy spool records are intentionally still readable without `source_run_id`, because old transaction recovery may have only session-level provenance. The finalization coverage audit previously treated such a `CONFIRMED` record as matching every completed run in the same session. In a repeat job, one ambiguous historic write-off could therefore make multiple `RUN_COMPLETED` records appear covered.

Fix:

```text
cc131a9fea585f9c1ffff003478bcd66666076a5
  Reject ambiguous legacy writeoff coverage
```

`CM_WireWriteOffCoverageAudit.cpp` now requires `source_run_id` before a legacy confirmed record can satisfy a concrete coverage target and then requires exact `source_run_id == run_id`. Ambiguous historic records remain readable but cannot be used as proof for `CLOSED`.

Regression:

```text
e4101b05c1fffea8c9ed22342c5ac746e4c6d1f3
  Guard exact-run finalization coverage
```

`RepairFinalizationGuard` maps coverage integrity failure to `WireWriteOffIntegrityFailed`, so ambiguous evidence blocks finalization instead of being treated as covered.

## Production write-off UI aligned with immutable spool provenance

After the backend provenance fix, the production write-off controller still offered `Без привязки к бухте` as a fallback when the immutable selected spool was no longer ACTIVE. That choice contradicted the current linked-job preparation model.

UI/controller fixes:

```text
a9960fd7a685b15f250f441f51f8f02ed4ab463d
  Align writeoff UI with immutable spool provenance

30bb618776ed7e9fedaad11409d20f90cbb7060c
  Align KG-first UI contract with exact spool

76bee056d989d99bff2c6152b9a486e6a9fdadcb
  Hide obsolete unallocated writeoff choice

77b107df943dd9f90cb10b25209f7e21da2ee3ed
  Hide obsolete unallocated writeoff choice
```

For new production write-offs the controller now loads the exact immutable session selection, requires the same selected spool to remain an ACTIVE material spool, always sends that exact `spool_id`, and blocks submission if it is unavailable instead of changing provenance after `RUN_COMPLETED`.

Desktop and mobile static forms show only the immutable source-session spool mode. Historical `UNALLOCATED` KG_FIRST records remain renderable in history.

A future real unallocated production workflow, if needed, must be modeled as an immutable material selection **before** the UART boundary. It must not be recreated as a post-run fallback that discards an already selected spool id.

## KG_FIRST HTTP semantics aligned with exact spool — fixed

After the store/UI fixes, the POST handler still exposed the old unallocated request shape: missing `spool_id` was accepted at parsing time and replaced by caller-supplied `diameter_hundredths_mm + wire_type`, while the immutable selection comparison was conditional on a non-zero spool id.

Fix:

```text
d4bb341e2646c78fb6dc9dd70a1899889fa0378e
  Require exact spool in KG-first API
```

For the current `POST /api/warehouse/write-offs` KG_FIRST path:

- `quantity_kg` remains the operator-entered exact quantity;
- `spool_id` is mandatory and non-zero;
- missing/invalid spool returns `spool_id_required_for_kg_first`;
- `selection.spoolId` must equal the supplied spool unconditionally;
- caller-supplied material/diameter no longer substitutes for the immutable spool;
- successful current POST responses always report `stock_mode: SPOOL` with the exact spool id.

Regression:

```text
90318b818ee0b2b749bc7023468c5ebf75cfabbc
  Guard exact spool KG-first HTTP semantics
```

Read-only history, movement codec, recovery and backup persistence still retain historical dual-schema compatibility for old `UNALLOCATED` records. New POST creation no longer produces them.

## Missing selection could bypass closure coverage — fixed

A second finalization gap was found after tightening cross-store persistence. `WireWriteOffCoverageAudit` previously skipped a `RUN_COMPLETED` record when the whole spool-selection directory was missing or when the exact session selection file could not be found. Skipping the run meant it never became a coverage target, so the repair could potentially be reported as `Covered` despite lost immutable material provenance.

Fix:

```text
96d848a685c2ecc4cecc5a881af1e5d1f89180fb
  Fail closure on missing spool selection evidence
```

For any completed run examined by the repair finalization audit:

- missing spool-selection catalog is now `IntegrityFailed`;
- missing exact session selection is now `IntegrityFailed`;
- malformed/mismatched selection remains `IntegrityFailed`;
- the run can no longer disappear from the coverage target set.

Regression:

```text
475f2d54bf2a9b8893dd676eb7a878e00ef2a3e3
  Guard closure against missing spool selection
```

This complements the deep persistence audit: backup integrity and repair closure now both fail closed when post-preparation immutable spool-selection evidence is lost.

## Historical write-off compatibility rechecked

`CM_WarehouseWriteOffHistory.cpp` still serializes historical `SPOOL`, `LEGACY_SPOOL`, and `UNALLOCATED` confirmed records read-only, including nullable `spool_id` and nullable legacy `source_run_id` where applicable.

`CM_WarehousePersistenceIntegrityAudit.cpp` resolves a spool reference only when `record.hasSpoolId`. Historical unallocated records therefore remain valid backup evidence without inventing a spool, while every existing repair reference is still required to resolve exactly once.

The migration rule is therefore asymmetric by design:

- old evidence remains readable/auditable;
- new production mutations are exact-spool only.

## Transaction residue policy reviewed

`JobStateStore` deliberately fails closed when `.tmp` or `.bak` transaction residue exists. Its existing regression contract requires authoritative state rotation and commit ordering to remain `target -> backup`, verified temp -> target, committed target verification, then backup cleanup; it explicitly forbids erasing interrupted transaction residue as a shortcut.

`JobSnapshotStore` orphan `.json.tmp` behavior remains a resilience REVIEW item rather than a current safety defect: snapshot creation occurs before CREATED state and before the UART boundary, so such residue cannot itself start or resume hardware. Any recovery policy should be designed consistently with the deliberate state-store transaction semantics rather than adding ad-hoc automatic deletion.

## Current next review

Continue with:

1. exact run/material costing and closure provenance after the new selection-coverage guard;
2. remaining snapshot/state/selection crash-residue consistency review;
3. fresh applicable ESP32 Build + CMP Protocol Tests before declaring this post-GREEN batch verified.
