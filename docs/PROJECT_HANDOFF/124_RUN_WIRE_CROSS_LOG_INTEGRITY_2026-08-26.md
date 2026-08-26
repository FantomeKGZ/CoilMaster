# Checkpoint 124 — RUN_WIRE cross-log integrity

Date: **2026-08-26**  
Branch: **`cmp-protocol-v1`**

## Status

**GREEN**

This checkpoint adds a bounded read-only integrity layer for completed atomic `RUN_WIRE` accounting.

## New integrity boundary

`CM_RunWireAccountingIntegrityAudit` verifies that every completed `RUN_WIRE` Material Request movement has exactly one matching:

- MaterialLedger usage tagged with the same `RWI_TX=<transaction_ref>`;
- warehouse `KG_FIRST / SPOOL / CONFIRMED` movement tagged with the same transaction ref;
- immutable `JobSpoolSelection` for the exact source session;
- spool ↔ MaterialLedger bridge for the exact physical spool and warehouse item.

The audit validates exact agreement for:

```text
transaction_ref
repair_id
warehouse_item_id
source_session_id
source_run_id
spool_id (through immutable JobSpoolSelection)
CU | AL
wire_diameter_hundredths_mm
consumed grams
MaterialLedger quantity
currency
timestamp
```

The implementation uses a fixed `ReferenceBatchSize = 16` and does not allocate an unbounded in-memory index.

## Orphan / duplicate rejection

For completed accounting:

```text
RUN_WIRE Material Request movement count
== tagged MaterialLedger usage count
== tagged warehouse CONFIRMED count
```

Each transaction ref must resolve exactly once in each target log. Duplicate Ledger evidence, duplicate warehouse confirmed evidence, duplicate bridge identity, missing immutable spool selection or orphan `RWI_TX` evidence fails closed.

## Recovery ownership

`RunWireIssueCoordinator` remains the only owner of in-flight recovery.

If either exists:

```text
/data/workshop/run-wire-issue.pending.json
/data/workshop/run-wire-issue.pending.tmp
```

cross-log integrity returns failure instead of trying to infer a partially committed transaction.

## Integration

`WorkshopPersistenceIntegrityAudit::check()` now includes `RunWireAccountingIntegrityAudit::check(storage)` after the normal warehouse / persistent-id / winding-session integrity checks.

This makes broad persistence/backup integrity reject a completed state where the three accounting views disagree.

## Commits

```text
c66c99bc46e59f8fef97ff31806a5b1873a0f152  audit header
9448c250955664c7e82a5e69ba26a569d3b93fe7  cross-log audit implementation
eef4157ac13e09d5636faa49817baa5a63cfc794  workshop integrity integration
63ac31dc37f2542e3879466df9158312ac21a2f6  mandatory RUN_WIRE contract coverage
```

## Verified CI

```text
ESP32 Build #1560  32959482667  SUCCESS
ESP32 Build #1561  32959521066  SUCCESS
CMP Tests #3507    32959482741  SUCCESS
CMP Tests #3509    32959605104  SUCCESS
```

## Safety invariants unchanged

- `RUN_COMPLETED` remains non-mutating;
- material ISSUE remains explicit operator action;
- physical START remains local-only;
- ESP32/Web never controls SSR directly;
- no auto-resume after reboot;
- no automatic wire deduction;
- exact spool/session/run provenance remains mandatory.

## Next

Audit and enforce price convergence between the MaterialLedger wire item used by atomic `RUN_WIRE` and the standard warehouse KG price used by confirmed physical writeoff/costing, so one transaction cannot persist two different wire costs.
