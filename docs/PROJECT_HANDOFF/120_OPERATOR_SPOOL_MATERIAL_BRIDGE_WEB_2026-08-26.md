# Checkpoint 120 — Operator spool ↔ MaterialLedger bridge Web — 2026-08-26

Branch: `cmp-protocol-v1`

## Result

GREEN. The persisted spool/material bridge from checkpoints 118–119 now has an explicit operator-only production HTTP creation path.

## Production route

```text
POST /api/warehouse/spool-material-bridges
```

Owner/runtime registration:

```text
WarehouseWeb::begin()
  -> existing WarehouseStore
  -> existing MaterialLedger
  -> SpoolMaterialBridgeStore
  -> SpoolMaterialBridgeWeb
```

No second warehouse or material catalogue owner was introduced.

## Mutation contract

The request requires:

```text
spool_id
warehouse_item_id
confirm=1
linked_at
```

Before append, the server proves:

1. the physical `spool_id` exists and is ACTIVE;
2. the MaterialLedger `warehouse_item_id` exists and is ACTIVE;
3. the MaterialLedger item uses `unit=GRAM`;
4. structured wire metadata exists;
5. exact `CU|AL` matches the physical spool;
6. exact `diameter_hundredths_mm` matches the physical spool;
7. the spool is not already bridged.

`wire_type` and diameter are not trusted from caller input. They are copied from authoritative persisted spool/material state only after exact cross-checking.

Success appends only:

```text
/data/warehouse/spool-material-bridges.ndjson
```

The response explicitly reports:

```json
"stock_mutated": false
```

The endpoint does not call current spool writeoff, KG-first writeoff, MaterialLedger usage, add-spool/add-material, machine START, or RUN_COMPLETED behavior.

## Safety boundary

Unchanged:

- `RUN_COMPLETED` never deducts material automatically;
- warehouse stock mutation still requires explicit operator action;
- current linked-production manual writeoff remains exact `source_session_id + source_run_id + immutable spool_id` until the coordinated migration is complete;
- bridge creation is identity evidence only, not consumption evidence;
- Arduino remains the only physical START/SSR owner.

## Implementation

New production module:

```text
firmware/esp32/src/CM_SpoolMaterialBridgeWeb.h
firmware/esp32/src/CM_SpoolMaterialBridgeWeb.cpp
```

Production bootstrap integration:

```text
firmware/esp32/src/CM_WarehouseWeb.cpp
```

Important implementation commit:

```text
ac105bdc2cd8f6dfb5379033bae465bf8ca4460c
feat(warehouse): register operator spool bridge API
```

Final verification-trigger source commit:

```text
fa651e3e50a25df9489db24b6c71bd853171a9b8
docs(warehouse): mark spool bridge API non-stock
```

## Regression

Permanent contract audit:

```text
Tests/Web/check_spool_material_bridge_web.js
```

It is executed by the already-wired `Audit MaterialLedger wire metadata` CMP step through:

```text
Tests/Web/check_material_wire_metadata.js
```

The audit protects explicit confirmation, authoritative references, exact metadata match, duplicate rejection, append-only bridge mutation, production bootstrap ownership, and absence of stock/machine mutations.

## Verified CI

Exact final tree `fa651e3e50a25df9489db24b6c71bd853171a9b8`:

```text
CMP Protocol Tests #3455
run 32944119683
SUCCESS

ESP32 Build #1541
run 32944119688
SUCCESS
```

The CMP run completed all configured host/Web/safety audits successfully, including the bridge Web regression through `Audit MaterialLedger wire metadata`.

## NEXT

The bridge identity boundary is now available. The next coherent migration block is crash-safe, explicit-operator run-linked wire accounting toward Material Request `RUN_WIRE`:

```text
material_request_id
source_session_id + source_run_id
exact physical spool provenance through bridge
CU/AL + actual consumed weight
manual confirmation
pending/recovery transaction
```

Do not partially remove existing exact-spool writeoff/finalization requirements. The transition must be coordinated across warehouse/material movement, Material Request, costing, finalization, backup/integrity, reports, Web and tests before the old production contract can be retired.
