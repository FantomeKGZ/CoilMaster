# CoilMaster — Material Request ↔ MaterialLedger adapter and lookup

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **GREEN**

## Scope

Existing `MaterialLedger` remains the authoritative generic warehouse item catalog. No duplicate catalog was introduced.

This block adds the exact conversion contract required for Material Request and a single authoritative active-item lookup used by future request transactions.

## Canonical unit mapping

Material Request quantity is stored as `quantity_milli_units` (thousandths of request unit). Existing MaterialLedger stock is stored as `stock_quantity_milli` (thousandths of the declared ledger unit).

Exact adapter:

```text
Request KG  -> Ledger GRAM         scale x1000
Request L   -> Ledger MILLILITRE   scale x1000
Request PCS -> Ledger PIECE        scale x1
Request M   -> Ledger METRE        scale x1
Request M2  -> Ledger SQUARE_METRE scale x1
```

For KG/L, both ledger quantity and request-side unit cost are scaled by 1000 so integer cost arithmetic remains dimensionally correct.

Cost contract:

```text
cost_amount_minor = ROUND(quantity_milli_units * request_unit_cost_minor / 1000)
```

All conversion arithmetic is integer-only and checks uint32/uint64 overflow before multiplication.

`RUN_WIRE` remains KG-only. `PCS` remains whole-piece only at Material Request movement level.

## New adapter

```text
firmware/esp32/src/CM_MaterialRequestUnitAdapter.h
firmware/esp32/src/CM_MaterialRequestUnitAdapter.cpp
```

It returns:

```text
ledgerUnit
ledgerQuantityMilli
requestUnitCostMinor
costAmountMinor
```

and rejects incompatible unit combinations or overflow.

## Catalog active-state lookup

Added to `MaterialLedger`:

```text
MaterialItemState
loadActiveMaterialState(materialId, state, found)
```

The single scan validates:

- canonical NDJSON;
- monotonic material IDs;
- supported stored unit;
- current stock;
- current accounting price;
- currency;
- ACTIVE status;
- optional comment syntax.

Existing `loadActiveMaterialCurrency()` now reuses this authoritative scan instead of maintaining a second parser path.

## Commits

```text
2343a35bfafc026b831a05c299cfd9b41c8c1fc7  add unit adapter header
ba2541afaa0fa021af82345ad07a1f0676b67ee8  implement exact unit scaling
83d7b0af7d7da7f3e47c6ab76c06bf390af0f5f8  add adapter regression
9001718fe9e5a11e9f0cf84ec73a9a339d86ef6b  add M2 movement support
788172c447271776224b46dbfc686a3ed6e81759  wire adapter regression
923c47cfe3e38e07c02a20ea877fa4274d575a62  expose MaterialItemState API
79af4e43efcdba36505e7a93abf64f3a5d86c1ec  implement authoritative active-state lookup
6cfcf6c66a4134bbe9ce6d3ba8da0e631e376e0a  add lookup regression
f471c2193b3ad5cc1acac25df537b896ca5c1a9c  wire lookup regression
```

## Permanent regressions

```text
Tests/Web/check_material_request_unit_adapter.js
Tests/Web/check_material_catalog_state_lookup.js
```

## Verified CI

Firmware implementation head:

```text
79af4e43efcdba36505e7a93abf64f3a5d86c1ec
```

Confirmed:

```text
CMP Protocol Tests run 32857435377 / SUCCESS
ESP32 Build run 32857318798 / SUCCESS
```

CMP run includes both permanent checks:

```text
Audit Material Request unit adapter
Audit material catalog active state lookup
```

## Safety impact

No stock mutation is added by this block. No Material Request runtime write API exists yet.

Still invariant:

- `RUN_COMPLETED` never automatically deducts material;
- warehouse ISSUE requires explicit operator action;
- exact session/run provenance remains mandatory for run-linked wire;
- old exact-spool production flow remains authoritative until full coordinated migration.

## Next

Add append-only Material Request status history and authoritative lifecycle resolver:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Then build a transactional coordinator for explicit operator ISSUE/RETURN/CORRECTION that couples MaterialLedger stock mutation with durable Material Request movement evidence and crash recovery.
