# Checkpoint 117 — Wire accounting forensic owner map

Date: **2026-08-26**  
Branch: **`cmp-protocol-v1`**

## Status

**DESIGN / FORENSIC CHECKPOINT. No runtime migration has been applied yet.**

The current exact-spool contract remains authoritative and must not be relaxed until a coherent compatibility transition is complete.

## Current physical spool owner

`WarehouseStore` owns physical wire spools:

```text
/data/warehouse/spools.ndjson
/data/warehouse/movements.ndjson
```

Current spool identity contains:

```text
spool_id
diameter_hundredths_mm
current_weight_g
wire_type = CU | AL
```

There is currently no `warehouse_item_id` / MaterialLedger identity in the physical spool schema.

## Immutable job spool selection

`JobSpoolSelectionStore` persists:

```text
job_id
session_id
repair_id
motor_id
spool_id
diameter_hundredths_mm
weight_at_selection_g
wire_type
```

Path:

```text
/data/winding-jobs/spool-selection/session-<session_id>.json
```

Selection is immutable production provenance. It explicitly requires non-zero `spool_id` and contains no MaterialLedger `warehouse_item_id`.

## Current manual linked wire writeoff

`POST /api/warehouse/write-offs` remains the current production writeoff path.

For linked completed runs it requires:

```text
repair_id
spool_id
source_session_id
source_run_id
actual weight/quantity
```

The handler:

1. requires repair OPEN;
2. loads immutable session spool selection;
3. requires exact `selection.repairId == repair_id`;
4. requires exact `selection.spoolId == spool_id`;
5. validates exact completed `source_session_id + source_run_id`;
6. rejects duplicate confirmed writeoff for the same source run;
7. mutates physical spool stock and writes warehouse movement evidence.

This path must remain intact until migration is complete.

## Current finalization coverage owner

`RepairFinalizationGuard` calls `WireWriteOffCoverageAudit`.

`WireWriteOffCoverageAudit` treats every linked `RUN_COMPLETED` as requiring exact writeoff coverage. It reads:

```text
winding journal
immutable JobSpoolSelection
/data/warehouse/movements.ndjson
```

Coverage target is currently:

```text
session_id + run_id + selected spool_id
```

Missing selection evidence is integrity failure, not a reason to waive writeoff.

## Current Material Request RUN_WIRE owner

`MaterialRequestWeb` and `MaterialRequestWarehouseCoordinator` support:

```text
source_kind = RUN_WIRE
movement_kind = ISSUE
unit = KG
material_request_id
repair_id
warehouse_item_id
source_session_id
source_run_id
material_class = CU | AL
wire_diameter_hundredths_mm
quantity_milli_units
```

The coordinator mutates `MaterialLedger` using `warehouse_item_id` and records crash-safe Material Request movement + ledger evidence.

## Critical identity gap

The two stock domains currently have no authoritative bridge:

```text
physical spool domain:
  spool_id
  exact remaining spool weight
  immutable job spool selection

MaterialLedger domain:
  warehouse_item_id / material_id
  generic stock_quantity_milli
  generic unit price
```

Current `RUN_WIRE` movement schema has no `spool_id`.
Current `JobSpoolSelection` / `WarehouseStore` spool schema has no `warehouse_item_id`.

Therefore directly replacing `spool_id` writeoff with current Material Request `RUN_WIRE` would lose physical spool traceability and could allow MaterialLedger stock mutation unrelated to the spool selected for the production session.

## Required first migration step

Before changing writeoff/finalization behavior, introduce an explicit fail-closed bridge between the selected physical spool and the MaterialLedger item.

Target bridge properties:

```text
spool_id
warehouse_item_id
wire_type/material_class
diameter_hundredths_mm
```

The bridge must be immutable or append-only/auditable for production use and must prevent:

- one run from charging a MaterialLedger item unrelated to the selected spool;
- CU/AL mismatch;
- diameter mismatch;
- silent fallback when bridge evidence is absent or malformed;
- auto writeoff on RUN_COMPLETED.

## Migration sequence

1. Define and persist physical spool ↔ MaterialLedger item bridge.
2. Add read/integrity/backup coverage for bridge evidence.
3. Extend immutable job spool selection or linked preparation evidence so exact bridge identity is frozen before UART.
4. Make Material Request RUN_WIRE ISSUE validate exact selected spool + bridge + session/run + repair.
5. Add crash-safe stock consistency rules covering both physical spool weight and MaterialLedger quantity; do not mutate only one side without transaction/recovery evidence.
6. Update costing/finalization to accept the new exact Material Request RUN_WIRE evidence only when all identity/provenance checks pass.
7. Update reports/Web/regressions/backup/integrity.
8. Only then retire the old linked writeoff path for new production sessions, while retaining historical read/audit compatibility.

## Safety invariants

Unchanged:

- `RUN_COMPLETED` is non-mutating;
- operator confirmation is required for stock ISSUE;
- exact `source_session_id + source_run_id` provenance is mandatory;
- physical START local-only;
- ESP32/Web do not control SSR;
- current exact `spool_id` path stays authoritative until coordinated migration is complete;
- malformed/missing migration evidence fails closed.

## Next implementation target

Design the smallest explicit spool↔MaterialLedger bridge persistence/API contract and regression tests. Do not yet alter manual writeoff or finalization semantics.
