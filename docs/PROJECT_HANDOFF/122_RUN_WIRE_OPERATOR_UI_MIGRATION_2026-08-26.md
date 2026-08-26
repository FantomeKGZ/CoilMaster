# 122 — RUN_WIRE operator UI migration

Date: **2026-08-26**  
Branch: **`cmp-protocol-v1`**

## Status

**GREEN — software/operator Web contract complete.**

Verified implementation/test tree before this documentation checkpoint:

```text
operator UI commit       5c28fadd4a3d1ef8de272f677e2b2f53bfc77794
UI contract commit       10528b23336bebe30208a56e085d3d77aeb19af9
fault-contract fix       f8d25c1b5fb04bddbd0c2b93fca704f14a7b565f
CMP Protocol Tests #3489 32954794059 / SUCCESS
```

The first two migration runs (#3487/#3488) failed only because the older writeoff fault audit still required the former direct UI POST. The audit was updated to the atomic route; the full host suite then passed in #3489.

## What changed

The desktop/mobile manual wire writeoff form now uses the shared controller as an explicit operator path into the checkpoint-121 atomic transaction:

```text
operator opens repair writeoff
-> exact uncovered RUN_COMPLETED selected read-only
-> immutable source session/run spool resolved
-> exact spool <-> MaterialLedger bridge resolved
-> operator explicitly chooses DRAFT/ISSUED Material Request
-> operator enters actual consumed kg
-> POST /api/material-requests/warehouse
   confirmed=true
   movement_kind=ISSUE
   source_kind=RUN_WIRE
   unit=KG
   material_request_id
   repair_id
   warehouse_item_id
   quantity_milli_units = actual grams
   source_session_id + source_run_id
   exact spool_id
   material_class
   wire_diameter_hundredths_mm
-> RunWireIssueCoordinator owns durable mutation/recovery
```

The production UI no longer performs a direct mutating POST to `/api/warehouse/write-offs`.

The legacy warehouse writeoff endpoint remains available for compatibility, and its GET history/coverage remains used as authoritative confirmed physical-writeoff evidence. It is not restored as the active operator mutation path.

## Fail-closed operator guards

The writeoff button remains disabled unless all required evidence exists:

- repair is confirmed OPEN;
- exact `RUN_COMPLETED` has `source_session_id + source_run_id`;
- immutable spool selection exists and matches the active spool identity;
- spool is bridged to an exact active MaterialLedger item with matching `CU/AL + diameter`;
- operator explicitly selects a Material Request in `DRAFT` or `ISSUED`;
- actual consumed quantity is valid and lower than current physical spool weight.

Missing bridge, missing/ambiguous provenance, invalid Material Request status, closed repair, malformed pagination or inconsistent identities all block mutation.

## Compatibility and accounting boundary

After this checkpoint the intended production accounting path is:

```text
RUN_COMPLETED
  -> no stock mutation
explicit operator RUN_WIRE ISSUE
  -> one Material Request movement
  -> one MaterialLedger usage
  -> one standard confirmed physical spool writeoff
```

Costing/finalization/read history continue to consume the standard confirmed physical writeoff evidence, avoiding a second production accounting model.

## Tests

`check_kg_first_material_contracts.js` now enforces:

- desktop/mobile Material Request selector and atomic RUN_WIRE controls;
- active POST is `/api/material-requests/warehouse`;
- exact request/warehouse/session/run/spool/wire identity is sent;
- direct legacy writeoff mutation is absent from the production controller.

`check_writeoff_fault_contracts.js` now distinguishes:

- legacy backend compatibility/failure semantics;
- read-only legacy history/coverage in the UI;
- atomic RUN_WIRE as the only active operator mutation path;
- no automatic writeoff, START or resume behavior.

## Safety invariants preserved

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web does not directly control SSR;
- `RUN_COMPLETED` remains non-mutating;
- material deduction remains explicit operator action;
- exact `material_request_id + source_session_id + source_run_id + spool_id + warehouse_item_id` provenance is mandatory for the new path.

## Next block

1. Audit costing/finalization/report surfaces for double-accounting between MaterialLedger usage and standard physical writeoff evidence.
2. Expose/retain exact Material Request + warehouse item provenance where operator/report views need it.
3. Decide when the legacy direct mutating POST can be formally deprecated without removing historical read compatibility.
4. Continue repository optimization/integrity work; final two-board hardware E2E remains mandatory later.
