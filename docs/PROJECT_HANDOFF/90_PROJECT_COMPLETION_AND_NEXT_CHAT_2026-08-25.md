# CoilMaster — completion estimate and next-chat transfer

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**  
Repo: `FantomeKGZ/CoilMaster`

## Stable baseline / source rule

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable pre-CRM
stable-2026-08-25-pre-crm-redesign -> same commit
```

Вся разработка после snapshot идёт только в `cmp-protocol-v1`. `main` не использовать как source.

## Current architecture

```text
CLIENT -> MOTOR -> REPAIR -> immutable AS_RECEIVED
                     -> WORKING/STARTING versions/jobs
                     -> MATERIAL REQUEST
                          -> WAREHOUSE ISSUE/RETURN/CORRECTION
                          -> COSTING
                     -> CASH/PAYMENTS/BALANCE
                     -> COMPLETED -> DELIVERED
```

Authoritative design:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

## Software GREEN checkpoints

```text
97  Motor winding versions
98  Repair AS_RECEIVED
99  Runtime/read API
100 Repair intake pending transaction
102 Transactional POST /api/repairs
103 Material Request schema foundation
104 CRM backup/export + integrity
105 MaterialLedger serialization fix
106 Material Request ↔ MaterialLedger unit adapter + active item lookup
107 Material Request lifecycle + backup/integrity
108 Material Request warehouse pending + backup guard
109 Crash-safe warehouse coordinator
110 Material Request production runtime/Web API
111 Immutable repair delivery store/API + backup/integrity
112 Append-only cash payments/corrections + repair/client balance API + backup/integrity
113 Motor Web catalog-only + separate create + versioned motor card
114 Motor AS_RECEIVED comparison + role-aware linked WORKING/STARTING job flow
```

Latest verified block 114:

```text
CMP Protocol Tests 32934323481 / SUCCESS
ESP32 Build         32934092563 / SUCCESS
```

ESP32 evidence is on production-source commit `da5d7271ba69e373599360550e81e1cf860f7a1a`, after role-aware `main.cpp` and repeat validation were integrated.

## Motor Web — GREEN

Desktop Motor Web now provides:

- catalog-only `motors.html`;
- separate `motor-new.html`;
- current and historical versioned WORKING/STARTING roles;
- multi-conductor display;
- immutable AS_RECEIVED vs after-repair comparison;
- OPEN-repair links to the existing safe linked-job flow with `role=working|starting`;
- exact server-side role/program/repeat validation;
- legacy WORKING-only fallback;
- fail-closed missing STARTING;
- readonly linked program/repeat in UI;
- exact spool selection retained;
- no physical START or direct SSR control from Web.

Checkpoint: `114_MOTOR_WEB_ROLE_AWARE_LINKED_JOB_2026-08-26.md`.

## Material Request / warehouse

Production routes:

```text
POST /api/material-requests
GET  /api/material-requests?repair_id=...
GET  /api/material-requests/item?material_request_id=...
GET  /api/material-requests/movements?material_request_id=...
GET  /api/material-requests/status?material_request_id=...
POST /api/material-requests/status
POST /api/material-requests/warehouse
```

Server derives client/motor from repair; stock mutation requires explicit confirmation and crash-safe coordinator. Client-supplied material pricing is not accepted.

## Delivery

```text
/data/workshop/repair-deliveries.ndjson
GET/POST /api/repairs/delivery
```

Only CLOSED repair may be delivered. Delivery is immutable, one per repair, preserves exact repair/client/motor/time and is intentionally not blocked by debt.

## Cash / payments

Authoritative repair charge stays in `RepairCosting`; cash journal does not duplicate price.

```text
/data/workshop/repair-payments.ndjson
PAYMENT -> ADD
CORRECTION -> ADD | SUBTRACT
```

Routes:

```text
GET  /api/payments?repair_id=...
GET  /api/payments?client_id=...
POST /api/payments
GET  /api/payments/balance?repair_id=...
GET  /api/payments/balance?client_id=...
```

Cash is append-only. Correction references must match the same repair/client. SUBTRACT cannot make paid total negative. Payment remains possible after CLOSED/DELIVERED. Backup/export/integrity includes both delivery and payment journals.

## Current mandatory work — Web/CRM UI

1. Client Web:
   - separate `client-new.html`;
   - catalog-only `clients.html`;
   - `client-details.html` with motors through repair history, open/closed repairs, materials, payments/balance and delivery history;
   - remove duplicated inline client creation from repairs page and leave navigation to the dedicated create page.
2. Dedicated `cash.html`; `costing.html` remains costing/pricing.
3. Coordinated spool -> Material Request wire migration only after UI/domain work and only across all affected contracts together.
4. Full software regression/backup/restore.
5. Final two-board hardware E2E.

## Wire accounting target

```text
RUN_COMPLETED -> never auto-deducts
operator explicitly confirms warehouse ISSUE
material_request_id
source_session_id + source_run_id for run-linked wire
CU/AL + actual weight
```

Current exact `spool_id` requirements remain until the whole chain is migrated coherently.

## Safety invariants

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never auto-deducts material;
- warehouse ISSUE requires explicit operator action;
- cash events never control machine/warehouse state;
- payment balance does not rewrite delivery/run/material evidence;
- cancellation/operator abort preserves immutable history;
- restore operator-only, transactional, fail-closed;
- no automatic production deletion/truncation.

## Hardware acceptance

Not complete. Full two-board hardware E2E remains mandatory after CRM/material/writeoff contracts stabilize.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/114_MOTOR_WEB_ROLE_AWARE_LINKED_JOB_2026-08-26.md
docs/PROJECT_HANDOFF/113_MOTOR_WEB_CATALOG_AND_VERSIONED_CARD_2026-08-26.md
docs/PROJECT_HANDOFF/112_CASH_PAYMENT_LEDGER_AND_BALANCE_API_2026-08-26.md
docs/PROJECT_HANDOFF/111_REPAIR_DELIVERY_STORE_API_2026-08-26.md
docs/PROJECT_HANDOFF/110_MATERIAL_REQUEST_RUNTIME_WEB_API_2026-08-26.md
docs/PROJECT_HANDOFF/109_MATERIAL_REQUEST_WAREHOUSE_COORDINATOR_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
this file
```

## Continuation prompt

```text
Продолжаем CoilMaster. Source-of-truth только cmp-protocol-v1; main не использовать. Checkpoint 114 Motor Web role-aware flow GREEN: CMP 32934323481 SUCCESS, ESP32 32934092563 SUCCESS. Motor catalog/create/version history/AS_RECEIVED/direct role navigation закрыты; linked job сервер проверяет exact role/program/repeat и exact spool, физический START остаётся только локальным. Следующий блок: Client Web — clients.html catalog-only, separate client-new.html, client-details with motors through repairs, payments/balance and delivery history. Потом cash.html. RUN_COMPLETED ничего автоматически не списывает; exact spool contract пока не удалять.
```
