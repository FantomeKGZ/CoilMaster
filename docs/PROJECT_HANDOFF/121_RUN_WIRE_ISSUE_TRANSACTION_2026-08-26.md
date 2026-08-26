# Checkpoint 121 — crash-safe RUN_WIRE ISSUE transaction

Date: **2026-08-26**  
Branch: **`cmp-protocol-v1`**

## Status

**GREEN.** Explicit operator RUN_WIRE ISSUE now owns one durable cross-store transaction boundary spanning Material Request evidence, MaterialLedger usage and exact physical spool writeoff.

Final verified source/test state before this documentation checkpoint:

```text
source commit: db643d33cd5327556429e71f3734864c484d2f40
final test commit: 7e73e9016c690e3ec65dfacfe3a80328b05a2148
ESP32 Build #1551 / run 32951550134 / SUCCESS
CMP Protocol Tests #3475 / run 32951582879 / SUCCESS
```

## Operator API contract

Production route remains:

```text
POST /api/material-requests/warehouse
```

RUN_WIRE requires explicit operator confirmation plus:

```text
confirmed=true
material_request_id=<id>
repair_id=<id>
warehouse_item_id=<id>
movement_kind=ISSUE
source_kind=RUN_WIRE
unit=KG
quantity_milli_units=<actual consumed grams>
source_session_id=<exact session>
source_run_id=<exact run>
spool_id=<exact physical spool>
material_class=CU|AL
wire_diameter_hundredths_mm=<exact diameter>
created_at=<timestamp>
comment=<optional>
```

For RUN_WIRE `unit=KG`, one request milli-unit is exactly one gram, so `quantity_milli_units` is the actual consumed wire weight in grams.

RUN_WIRE returns before the generic Material Request warehouse coordinator path. It therefore cannot silently fall back to a MaterialLedger-only mutation.

## Authoritative validation

Before any mutation, `RunWireIssueCoordinator` requires all of the following:

- Material Request exists and belongs to the exact repair;
- Material Request status allows warehouse mutation (`DRAFT` or `ISSUED`);
- repair is OPEN;
- immutable `JobSpoolSelection` exists for the exact `source_session_id`;
- selection matches exact repair, `spool_id`, CU/AL and diameter;
- `WindingSessionCompletionAudit` proves the exact `source_session_id + source_run_id` completed;
- spool-material bridge exists for the exact physical spool;
- bridge points to the requested MaterialLedger item;
- bridge CU/AL + diameter matches both authoritative domains;
- MaterialLedger item is ACTIVE, `GRAM`, KGS, and has exact structured wire metadata;
- physical spool is ACTIVE and has enough weight;
- no confirmed physical writeoff already exists for the exact source session/run.

## One durable transaction owner

High-level authoritative intent:

```text
/data/workshop/run-wire-issue.pending.json
```

Atomic temp path:

```text
/data/workshop/run-wire-issue.pending.tmp
```

The pending record freezes:

```text
transaction_ref
material_request_id
repair_id
warehouse_item_id
source_session_id
source_run_id
spool_id
consumed_grams
spool_weight_before_g
spool_weight_after_g
ledger_quantity_milli
unit/cost/currency evidence
CU|AL
diameter
created_at
comment
```

Execution order:

```text
1. durable RUN_WIRE pending
2. immutable Material Request RUN_WIRE ISSUE movement
3. MaterialLedger repair usage tagged RWI_TX=<transaction_ref>
4. subordinate warehouse PENDING evidence
5. exact physical spool before -> after mutation
6. subordinate warehouse CONFIRMED evidence
7. verify all evidence + exact after-state
8. clear authoritative RUN_WIRE pending
```

MaterialLedger and Warehouse retain their own low-level atomic recovery files/records, but they are subordinate storage phases. `RunWireIssueCoordinator` is the single business-level recovery owner.

## Recovery semantics

Recovery inspects four independent durable facts:

```text
Material Request movement evidence
MaterialLedger RWI_TX usage evidence
standard warehouse CONFIRMED writeoff evidence
exact physical spool before/after weight
```

Valid recovery progression is monotonic:

```text
pending only + exact before -> clear unused intent
movement only -> replay Ledger
movement + Ledger + exact before -> retry physical warehouse phases
movement + Ledger + warehouse CONFIRMED + exact after -> clear pending
```

Impossible ordering or unexpected spool weight fails closed. Recovery never guesses a stock state.

Existing `WarehouseStore::begin()` reconciles any subordinate dangling warehouse PENDING first. Its deterministic recovery semantics remain authoritative: before-state closes the subordinate attempt as ABORTED, after-state closes it as CONFIRMED, and any unknown state fails closed.

## Existing writeoff/finalization compatibility

The new transaction still emits the existing standard warehouse KG_FIRST `CONFIRMED` evidence tied to exact:

```text
spool_id
repair_id
source_session_id
source_run_id
```

Therefore existing writeoff lookup, coverage audit, costing and repair finalization continue to see the same authoritative physical writeoff evidence rather than a parallel replacement format.

## Backup / restore safety

`BackupActivityGuard` now returns `Busy` while either RUN_WIRE pending path exists. Backup/restore cannot cross an unfinished cross-store stock transaction boundary.

The pending file itself is recovery intent, not healthy backup payload: a healthy transaction clears it before backup becomes Safe.

## Tests

`Tests/Web/check_run_wire_issue_transaction.js` locks:

- authoritative pending fields;
- ISSUE/RUN_WIRE/KG-only contract;
- exact immutable spool selection;
- exact completed run;
- bridge + MaterialLedger wire identity;
- durable execution order;
- fail-closed recovery ordering;
- subordinate warehouse PENDING/CONFIRMED evidence;
- idempotent exact spool replay;
- mandatory Web `spool_id`;
- early return before generic Ledger-only coordinator;
- runtime recovery gate.

The test is required from the existing mandatory Material Request warehouse CMP contract step. Restore mutation tests additionally lock the RUN_WIRE pending backup interlock.

## Safety invariants preserved

- `RUN_COMPLETED` remains strictly non-mutating;
- stock changes only after explicit operator `confirmed=true` RUN_WIRE ISSUE;
- no automatic physical START;
- no automatic restart/resume after reboot;
- ESP32/Web has no direct SSR authority;
- exact `spool_id + source_session_id + source_run_id` provenance remains mandatory;
- cancellation/abort does not erase immutable evidence;
- recovery and backup/restore remain fail-closed.

## Next software block

Audit downstream Material Request / wire reporting and operator UI so the new atomic RUN_WIRE ISSUE is visible and usable end-to-end without weakening the existing exact-spool finalization contract. Keep the legacy exact-spool direct writeoff endpoint available until that operator/report migration is coherently GREEN.
