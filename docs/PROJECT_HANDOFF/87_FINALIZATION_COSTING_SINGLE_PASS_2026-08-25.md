# 87 — Finalization costing single-pass movement audit — 2026-08-25

## Scope

Stage-1 ESP32/storage performance cleanup after the GREEN workshop winding single-pass block.

`RepairFinalizationGuard::check()` previously validated `/data/warehouse/movements.ndjson` twice during one finalization preflight:

1. direct `WarehouseMovementIntegrityAudit::check(storage)`;
2. `RepairCosting::load(repairId, summary)`, which calls `WarehouseMovementIntegrityAudit::checkRepair(...)` and therefore performs the same authoritative transaction/provenance validation again while aggregating exact repair wire totals.

The direct pre-pass is removed. Finalization now relies on the costing load's authoritative movement audit and aggregation pass.

## Preserved integrity

`WarehouseMovementIntegrityAudit::checkRepair()` delegates to the same `checkInternal()` implementation as the broad `check()` overload. It still validates:

- full write-off record parsing;
- monotonic movement IDs;
- exact PENDING -> terminal transaction pairing;
- transaction core equality;
- no dangling PENDING record;
- confirmed provenance uniqueness via the existing bounded batch audit;
- checked wire-cost aggregation for the exact repair.

`RepairFinalizationGuard` still separately validates winding transitions and exact-run write-off coverage after costing.

No physical START, SSR, UART, Hall, run-completion, exact-spool, or manual wire write-off semantics changed.

## Deliberate non-change

`confirmedProvenanceUnique()` retains its bounded batch rescans. That is an intentional bounded-memory uniqueness strategy. Do not replace it with unbounded RAM state or storage-engine migration without populated-media measurements.

## Regression / CI

- `Tests/Web/check_finalization_costing_single_pass.js`
- CMP workflow step: `Audit finalization costing single-pass contracts`

Required software verification:

- ESP32 Build SUCCESS on `b6e9646c...` or descendant;
- CMP Protocol Tests SUCCESS on `d37a558a...` or descendant;
- explicit `Audit finalization costing single-pass contracts` SUCCESS;
- existing `Audit write-off fault contracts`, `Audit final acceptance contracts`, and kg-first contracts remain SUCCESS.

Hardware testing is not required for this repo-only optimization block.
