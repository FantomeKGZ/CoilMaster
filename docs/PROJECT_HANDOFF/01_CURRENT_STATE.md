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
                     -> CASH/PAYMENTS/BALANCE
                     -> COMPLETED -> DELIVERED
```

Warehouse = physical materials. Cash = money. Material Request bridges repair/warehouse/costing.

## Software GREEN checkpoints

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
111 Immutable repair delivery store/API + backup/integrity
112 Append-only cash/payment journal + repair/client balance API + backup/integrity
113 Motor Web catalog-only + separate creation + versioned working card
114 Immutable AS_RECEIVED comparison + role-aware linked WORKING/STARTING job flow
```

Latest checkpoint 114 evidence:

```text
CMP Protocol Tests 32934323481 / SUCCESS
ESP32 Build         32934092563 / SUCCESS
```

The ESP32 run is on production-source commit `da5d7271ba69e373599360550e81e1cf860f7a1a`, after the guarded `main.cpp` role/repeat integration. Later commits only tighten Web/host regressions and documentation.

## Motor Web — SOFTWARE GREEN

Desktop Motor Web now has:

- catalog-only `/desktop/motors.html`;
- separate `/desktop/motor-new.html`;
- `/desktop/motor-details.html` with current WORKING/STARTING version and bounded version history;
- canonical multi-conductor display;
- immutable `AS_RECEIVED` vs exact after-repair version comparison using `source_repair_id`;
- explicit legacy fallback rather than pretending missing historical evidence exists;
- safe OPEN-repair links to `/desktop/winding-job.html?repair_id=...&role=working|starting`;
- CLOSED repairs do not expose role send links.

Linked production job validation is server-owned:

```text
repair -> exact motor -> OPEN
latest winding version
  -> exact role
  -> exact program
  -> exact repeat_target
exact spool selection
snapshot -> state -> spool selection -> DELIVERING -> UART
```

Legacy motor fallback authorizes WORKING only. Versionless STARTING fails closed. Present but malformed legacy repeat data also fails closed. `winding-job.html` treats program and repeat target as readonly convenience data and the server revalidates both before persistence/UART.

Physical START remains local-only and Web never controls SSR.

## Material / warehouse model

`/data/materials/materials.ndjson` and `MaterialLedger` remain the authoritative generic warehouse catalog.

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

Every movement has `transaction_ref`. `RUN_WIRE` is ISSUE/KG-only and requires exact `source_session_id + source_run_id`, CU/AL and diameter. `RUN_COMPLETED` remains non-mutating.

## Delivery model — GREEN

```text
/data/workshop/repair-deliveries.ndjson
GET/POST /api/repairs/delivery
```

Delivery is a separate immutable fact from repair CLOSED. Only a CLOSED repair can be delivered; exact repair/client/motor/time are preserved; one final delivery per repair; balance/debt does not block delivery.

## Cash/payment model — GREEN

Authoritative charge remains `RepairCosting::load()` / `/data/repairs/pricing.ndjson`. Cash never duplicates repair price.

Append-only journal:

```text
/data/workshop/repair-payments.ndjson
```

Events:

```text
PAYMENT    -> ADD only
CORRECTION -> ADD | SUBTRACT
```

Correction target must belong to the same repair/client. SUBTRACT cannot drive paid total below zero. Payments remain possible after CLOSED/DELIVERED.

API:

```text
GET  /api/payments?repair_id=...
GET  /api/payments?client_id=...
POST /api/payments
GET  /api/payments/balance?repair_id=...
GET  /api/payments/balance?client_id=...
```

Balances expose charged/paid/debt/credit. Client aggregate charge is calculated by bounded repair paging and authoritative RepairCosting reads; payment aggregate is one pass over the cash journal.

Backup includes repair deliveries and repair payments, with fail-closed integrity audits.

## Current NEXT

1. Client Web redesign:
   - separate `client-new.html`;
   - catalog-only `clients.html`;
   - `client-details.html` with motors through repair history, open/closed repairs, material requests, payments/balance and delivery history;
   - remove duplicated inline client creation from repair intake and leave a link/button to `client-new.html`.
2. Dedicated `cash.html`; keep `costing.html` for cost/price/margin only.
3. Coordinated spool -> Material Request wire migration only after job/writeoff/finalization/backup/report/Web/tests are changed coherently.
4. Full Web/backend regression + backup/restore, then two-board hardware E2E.

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
- cash operations never trigger machine or warehouse mutation;
- run-linked wire movement preserves exact run provenance;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Hardware acceptance

Not complete. Full two-board E2E remains mandatory after CRM/material/writeoff contracts stabilize.

## Documentation rule

Synchronize 95/101/06/01/90 and update 00 when read order changes. Major persistence/API/UI blocks receive numbered checkpoints with exact commit and CI evidence.
