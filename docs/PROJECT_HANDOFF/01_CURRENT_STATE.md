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
```

Latest CRM backup/integrity verification:

```text
CMP run 32855540935 / SUCCESS
ESP32 Build run 32855541246 / SUCCESS
```

New CRM journals are exported and deep-audited. Repair-intake pending/temp markers block stable backups. `CM_CrmPersistenceIntegrityAudit` is read-only and fail-closed.

## Current implemented Material Request foundation

Stores:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
```

Request identity keeps:

```text
material_request_id + repair_id + client_id + motor_id
initial_status DRAFT
```

Movement contract supports:

```text
ISSUE | RETURN | CORRECTION
MANUAL_MATERIAL | RUN_WIRE
KG | L | PCS | M
integer quantity_milli_units
unit_cost_minor + cost_amount_minor + currency
```

`RUN_WIRE` requires exact `source_session_id + source_run_id`, CU/AL, diameter and KG. `MANUAL_MATERIAL` cannot fake run provenance.

No Material Request runtime mutation API exists yet. Existing exact-spool/writeoff flow is still authoritative.

## Current NEXT

The existing `MaterialLedger` will be reused as generic warehouse item catalog rather than introducing a duplicate catalog. Before reuse, fix and regression-protect `MaterialLedger::addMaterial()` unit JSON serialization, then define canonical unit mapping and warehouse item lookup/state for Material Request. After that implement `DRAFT -> ISSUED -> PRICED -> CLOSED` and explicit operator ISSUE/RETURN/CORRECTION APIs.

## Wire migration

Future contract:

```text
RUN_COMPLETED -> never auto-deducts
operator confirms warehouse ISSUE
material_request_id
exact source_session_id + source_run_id for wire
CU/AL + actual consumed weight
```

Current exact `spool_id` backend/finalization safety checks remain until all job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests are migrated coherently.

## Web target

Motor:
- `motor-new.html` separate;
- `motors.html` catalog-only in Arduino archive style;
- `motor-details.html` working card with winding versions and direct WORKING/STARTING JOB send;
- repair/material-request history.

Client:
- `client-new.html` separate;
- `clients.html` catalog-only;
- `client-details.html` with motors, repairs, requests, payments/balance, delivered date.

Cash:
- separate financial subsystem from warehouse/costing;
- partial/multiple payments, corrections/refunds, debt/overpayment;
- payment ↔ repair ↔ material request ↔ warehouse navigation.

## Repair lifecycle

AS_RECEIVED immutable. Repair CLOSED != delivered. Delivery evidence append-only. Debt warns but does not hard-block after explicit operator confirmation.

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
