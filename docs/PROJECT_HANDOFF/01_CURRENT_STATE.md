# Текущее состояние CoilMaster

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

## Source of truth / stable baseline

Working source-of-truth only `cmp-protocol-v1`. `main` не использовать как source.

```text
stable pre-CRM: 449570d47649d5f6336a31ee3eed491256e0fb1a
main -> same commit
stable-2026-08-25-pre-crm-redesign -> same commit
```

## Current phase

Workshop Web/CRM redesign.

Authoritative design:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

Active queue: `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

Target flow:

```text
CLIENT -> MOTOR -> REPAIR -> AS_RECEIVED
                     -> WINDING VERSION/JOBS
                     -> MATERIAL REQUEST
                          -> WAREHOUSE physical movements
                          -> COSTING
                     -> CASH/PAYMENTS
                     -> COMPLETED -> DELIVERED
```

Warehouse = physical materials. Cash = money. Material Request bridges repair/warehouse/costing.

## Phase A software GREEN blocks

```text
97  Motor winding versions
98  Repair AS_RECEIVED persistence
99  Runtime/read winding + snapshot API
100 Repair intake pending transaction foundation
102 Transactional repair creation + crash recovery
103 Material Request identity + movement schema foundation
104 CRM backup/export + integrity
105 MaterialLedger catalog serialization fix
106 Material Request ↔ MaterialLedger unit adapter + active item lookup
```

Latest catalog foundation verification:

```text
CMP run 32857435377 / SUCCESS
ESP32 Build run 32857318798 / SUCCESS
```

## Material / warehouse catalog foundation

Existing `/data/materials/materials.ndjson` and `MaterialLedger` are authoritative generic warehouse item catalog. A duplicate catalog is not being introduced.

Catalog serialization bug is fixed and permanent regression-protected.

Canonical Material Request unit mapping:

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

Conversion and costing are integer-only with overflow guards.

`MaterialLedger::loadActiveMaterialState()` now supplies exact ACTIVE item state:

```text
material_id
unit
stock_quantity_milli
price_per_unit_minor
currency
```

Existing currency lookup reuses the same authoritative scan.

## Material Request current implementation

Stores:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
```

Movements support:

```text
ISSUE | RETURN | CORRECTION
MANUAL_MATERIAL | RUN_WIRE
KG | L | PCS | M | M2
```

`RUN_WIRE` remains KG-only and requires exact `source_session_id + source_run_id`, CU/AL and diameter.

No Material Request runtime stock mutation API exists yet. Existing exact-spool/writeoff flow remains authoritative.

## Current NEXT

Implement append-only Material Request status history + resolver for:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Then add backup/integrity for status history and a crash-safe transaction coordinator for explicit operator ISSUE/RETURN/CORRECTION. `RUN_COMPLETED` must remain non-mutating.

## Wire migration

Future contract:

```text
RUN_COMPLETED -> never auto-deducts
operator confirms warehouse ISSUE
material_request_id
exact source_session_id + source_run_id for wire
CU/AL + actual consumed weight
```

Current exact `spool_id` backend/finalization checks remain until coherent migration across job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests.

## Safety invariants

Never weaken:

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts material;
- warehouse ISSUE requires explicit operator action;
- run-linked wire movement preserves exact run provenance;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Hardware acceptance

Not complete. A full two-board E2E pass remains mandatory after final CRM/material/writeoff contracts stabilize.

## Documentation rule

Synchronize 95/101/06/01/90 and update 00 when read order changes. New major persistence/API blocks receive numbered checkpoints with exact commit and CI evidence.
