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
115 Client Web catalog-only + dedicated create + read-only CRM card
```

Latest evidence:

```text
CMP Protocol Tests 32936343060 / SUCCESS
ESP32 Build         32934092563 / SUCCESS  # latest firmware-source evidence from checkpoint 114
```

Checkpoint 115 changed Web/host regression files only; firmware-source evidence therefore remains checkpoint 114.

## Motor Web — SOFTWARE GREEN

Desktop Motor Web now has catalog-only list, separate create page, versioned WORKING/STARTING card/history, AS_RECEIVED comparison, safe role navigation to existing linked-job flow, exact role/program/repeat server validation, exact spool retention, and no Web physical START/SSR shortcut.

## Client Web — SOFTWARE GREEN

Checkpoint: `115_CLIENT_WEB_CRM_2026-08-26.md`.

Desktop Client Web now has:

- catalog-only `/desktop/clients.html` with bounded paging/search;
- separate `/desktop/client-new.html`;
- `/desktop/client-details.html?client_id=...`;
- exact client identity/contact/comment;
- repair history paged by exact client ID;
- motors resolved historically from each repair's exact `motor_id`;
- OPEN/CLOSED state;
- immutable delivery state/date;
- client charged / paid / debt / credit;
- bounded append-only payment history;
- no client/payment/delivery/machine/material mutation from the client card;
- no duplicate inline client creation in repair intake.

A physical motor remains independent from current client identity. Client↔motor relationship is historical through repairs; no mutable permanent `client_id` is written into motor identity.

## Material / warehouse model

`/data/materials/materials.ndjson` and `MaterialLedger` remain the authoritative generic warehouse catalog.

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

Material Request lifecycle remains `DRAFT -> ISSUED -> PRICED -> CLOSED`; warehouse ISSUE/RETURN/CORRECTION are explicit operator actions. `RUN_WIRE` remains ISSUE/KG-only with exact `source_session_id + source_run_id`, CU/AL and diameter. `RUN_COMPLETED` remains non-mutating.

## Delivery model — GREEN

```text
/data/workshop/repair-deliveries.ndjson
GET/POST /api/repairs/delivery
```

Delivery is immutable and separate from repair CLOSED and from cash balance. Debt does not block delivery.

## Cash/payment model — GREEN backend

Authoritative charge remains `RepairCosting::load()` / `/data/repairs/pricing.ndjson`. Cash never duplicates repair price.

```text
/data/workshop/repair-payments.ndjson
PAYMENT    -> ADD only
CORRECTION -> ADD | SUBTRACT
```

API:

```text
GET  /api/payments?repair_id=...
GET  /api/payments?client_id=...
POST /api/payments
GET  /api/payments/balance?repair_id=...
GET  /api/payments/balance?client_id=...
```

Cash is append-only; correction references remain same-repair/client only; SUBTRACT cannot drive paid total below zero.

## Current NEXT

1. Dedicated `cash.html` using checkpoint 112 backend.
2. Keep `costing.html` strictly for cost/price/margin.
3. Cash UI: exact repair/client context, charged/paid/debt/credit, full/partial/multiple payments, append-only corrections, explicit confirmation, no edit/delete.
4. Coordinated spool -> Material Request wire migration only after job/writeoff/finalization/backup/report/Web/tests are changed coherently.
5. Full Web/backend regression + backup/restore, then two-board hardware E2E.

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
