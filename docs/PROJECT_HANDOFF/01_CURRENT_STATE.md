# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
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
107 Material Request append-only lifecycle + backup/integrity
108 Material Request warehouse pending persistence + stable-backup guard
109 Crash-safe Material Request warehouse coordinator
110 Material Request production runtime/Web API
```

Latest block 110 verification:

```text
CMP run 32926200712 / SUCCESS
ESP32 Build run 32926237400 / SUCCESS
runtime bootstrap ESP32 Build 32926105448 / SUCCESS
```

## Material / warehouse model

Existing `/data/materials/materials.ndjson` and `MaterialLedger` remain the authoritative generic warehouse catalog.

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

Material Request durable data:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
/data/workshop/material-request-status.ndjson
/data/workshop/material-request-warehouse.pending.json
/data/workshop/material-request-warehouse.pending.tmp
```

Lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Warehouse operations:

```text
ISSUE | RETURN | CORRECTION
CORRECTION -> ADD | REMOVE
MANUAL_MATERIAL | RUN_WIRE
KG | L | PCS | M | M2
```

Every movement has `transaction_ref`. `RUN_WIRE` is ISSUE/KG-only and requires exact `source_session_id + source_run_id`, CU/AL and diameter. `RUN_COMPLETED` is non-mutating.

Coordinator ordering:

```text
explicit operator confirmation
-> durable pending marker
-> immutable movement
-> physical MaterialLedger mutation
-> verify both sides
-> clear pending
```

Recovery is fail-closed on impossible ledger-only evidence. Warehouse mutation is allowed only for DRAFT/ISSUED request tied to an OPEN repair.

## Material Request production API — GREEN

```text
POST /api/material-requests
GET  /api/material-requests?repair_id=...
GET  /api/material-requests/item?material_request_id=...
GET  /api/material-requests/movements?material_request_id=...
GET  /api/material-requests/status?material_request_id=...
POST /api/material-requests/status
POST /api/material-requests/warehouse
```

Creation derives exact `client_id + motor_id` from authoritative `repair_id`. Warehouse mutation requires `confirmed=true`, does not accept operator-supplied material price and executes only through the coordinator. Stores and transaction recovery initialize before routes are registered.

## Current NEXT

1. Add immutable repair delivery store/API: repair completion and delivery are different facts.
2. Delivery record keeps exact `repair_id + client_id + motor_id + delivered_at`, with optional comment.
3. Delivery requires repair CLOSED but must not require balance == 0; a debt warning belongs to future Cash/UI, not the persistence gate.
4. Then add append-only payments/corrections and client/repair balance reads.
5. Then Motor Web / Client Web redesign using the new model.
6. Coordinated spool -> Material Request wire migration only after job/writeoff/finalization/backup/report contracts are changed together.

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

Not complete. Full two-board E2E remains mandatory after CRM/material/writeoff contracts stabilize.

## Documentation rule

Synchronize 95/101/06/01/90 and update 00 when read order changes. Major persistence/API blocks receive numbered checkpoints with exact commit and CI evidence.
