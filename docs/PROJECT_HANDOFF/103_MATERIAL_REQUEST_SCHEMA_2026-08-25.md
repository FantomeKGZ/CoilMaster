# CoilMaster — Material Request schema foundation

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN / persistence foundation**

Этот checkpoint закрывает первый implementation block по design `101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md`.

## Цель

Создать отдельные append-only production contracts, которые в дальнейшем свяжут:

```text
REPAIR -> MATERIAL REQUEST -> WAREHOUSE MOVEMENTS -> COSTING/CASH
```

Старый spool/writeoff flow в этом блоке не удаляется и не меняется.

## Material Request store

```text
/data/workshop/material-requests.ndjson
firmware/esp32/src/CM_MaterialRequestStore.h
firmware/esp32/src/CM_MaterialRequestStore.cpp
```

Identity:

```text
material_request_id
repair_id
client_id
motor_id
initial_status = DRAFT
created_at
comment optional
```

Properties:

- append-only `FILE_APPEND`;
- strictly increasing `material_request_id`;
- bounded read by exact request id;
- bounded paging by `repair_id`;
- no implicit archive copy or destructive rewrite.

## Material Request movement store

```text
/data/workshop/material-request-movements.ndjson
firmware/esp32/src/CM_MaterialRequestMovementStore.h
firmware/esp32/src/CM_MaterialRequestMovementStore.cpp
```

Movement identity/provenance:

```text
movement_id
material_request_id
repair_id
warehouse_item_id
movement_kind = ISSUE | RETURN | CORRECTION
source_kind = MANUAL_MATERIAL | RUN_WIRE
quantity_milli_units
unit
unit_cost_minor
cost_amount_minor
currency
created_at
comment optional
```

### Units

Bounded initial unit set:

```text
KG | L | PCS | M
```

`quantity_milli_units` uses integer thousandths of the selected unit:

```text
KG: 1 = 1 gram
L:  1 = 1 ml
PCS: only multiples of 1000 are valid
M:  1 = 0.001 m
```

This avoids floating-point accounting in persistence.

## RUN_WIRE provenance

`RUN_WIRE` additionally requires:

```text
source_session_id > 0
source_run_id > 0
material_class = CU | AL
wire_diameter_hundredths_mm > 0
unit = KG
```

`MANUAL_MATERIAL` requires run identity to remain zero and cannot fake wire-specific provenance.

This keeps the target invariant ready for the later coordinated spool migration:

```text
RUN_COMPLETED never auto-deducts material.
Operator explicitly confirms warehouse ISSUE.
Run-linked wire ISSUE preserves exact source_session_id + source_run_id.
```

## Cost snapshot

Each movement carries:

```text
unit_cost_minor
cost_amount_minor
currency
```

These are accounting snapshots for later costing/cash integration. Cash/payment events remain a separate future subsystem.

## Implementation commits

```text
c056011fddfbbc640ea5dffd419d9c7f1e746f92  feat(crm): add material request persistence contract
b04761ef20d8d940e4b4c16c125e60e523bb40b1  feat(crm): persist material request identities
b4b92188e2b663b9c3437db8c7f37bad31960176  feat(crm): add material request movement contract
a24c1e7e212f9d971bb7ad8896c3fdcf10aa2284  feat(crm): persist material request movements
36a18ed92b6f9fe016b0ab8965b861d39d59602c  test(crm): protect material request schema
10ce3c251920c006daa256485bb9f89768c43722  ci(crm): audit material request schema
```

## Verification

Implementation ESP32 build:

```text
ESP32 Build run 32851843400 / SUCCESS
head a24c1e7e212f9d971bb7ad8896c3fdcf10aa2284
```

Permanent regression:

```text
Tests/Web/check_material_request_schema.js
```

CMP workflow step:

```text
Audit material request schema contracts
```

Verified:

```text
CMP Protocol Tests #3189 / run 32852061125 / SUCCESS
head 10ce3c251920c006daa256485bb9f89768c43722
```

The same run also revalidated the existing transactional repair-intake and safety contracts.

## What is NOT implemented yet

- no runtime/Web API for creating requests/movements;
- no request status transition store yet;
- no generic warehouse item catalog yet;
- no physical stock decrement through this subsystem yet;
- no costing/cash consumer yet;
- no old spool/writeoff removal;
- no release-critical backup/restore integration yet.

Therefore these new journals are foundation only and cannot yet replace existing warehouse production paths.

## Next mandatory block

Before exposing Material Request as release-critical runtime mutation:

1. add motor winding versions, AS_RECEIVED, material requests and request movements to backup/export coverage;
2. treat repair-intake pending/temp files as recovery markers, not normal business backup payload;
3. add fail-closed CRM integrity audit with cross-reference validation;
4. then add generic warehouse item/unit catalog and request/status/API layers.

## Safety

Unchanged:

- no automatic physical START;
- Arduino owns SSR;
- `RUN_COMPLETED` never auto-deducts material;
- warehouse ISSUE requires explicit operator action;
- run-linked wire ISSUE must preserve exact session/run provenance;
- existing exact-spool contract remains authoritative until full migration;
- no destructive history rewrite;
- restore operator-only, transactional, fail-closed.
