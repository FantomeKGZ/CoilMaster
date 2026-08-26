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
115 Client Web catalog-only + dedicated create + read-only CRM card
```

Latest verified:

```text
CMP Protocol Tests 32936343060 / SUCCESS
ESP32 Build         32934092563 / SUCCESS  # latest firmware-source evidence from checkpoint 114
```

## Motor Web — GREEN

Desktop Motor Web provides catalog-only browsing, separate creation, versioned WORKING/STARTING, multi-conductor history, immutable AS_RECEIVED comparison and safe navigation to the existing linked-job path. Server owns exact role/program/repeat/spool validation. Physical START remains local-only.

Checkpoint: `114_MOTOR_WEB_ROLE_AWARE_LINKED_JOB_2026-08-26.md`.

## Client Web — GREEN

Checkpoint: `115_CLIENT_WEB_CRM_2026-08-26.md`.

Desktop Client Web now provides:

- catalog-only `clients.html`;
- separate `client-new.html`;
- `client-details.html?client_id=...`;
- client identity/contact/comment;
- motors resolved through exact repair history rather than permanent motor ownership;
- bounded open/closed repair history;
- immutable delivery state/date;
- charged/paid/debt/credit;
- bounded append-only payment history;
- no payment/delivery/machine/material mutation from client card;
- no duplicate inline client creation in repairs page.

## Material Request / warehouse

Production routes remain explicit and crash-safe. `RUN_COMPLETED` is non-mutating. Warehouse stock changes only through operator-confirmed Material Request ISSUE/RETURN/CORRECTION. Current exact spool production contract remains authoritative.

## Delivery

```text
/data/workshop/repair-deliveries.ndjson
GET/POST /api/repairs/delivery
```

Only CLOSED repair may be delivered. Delivery is immutable and intentionally not blocked by debt.

## Cash / payments backend

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

Cash is append-only. Repair pricing remains authoritative for charge; cash does not duplicate price.

## Current mandatory work

1. Dedicated `cash.html` using checkpoint 112 APIs.
2. Keep `costing.html` for cost/price/margin only.
3. Cash UI must support exact repair/client context, balances, full/partial/multiple payments, append-only corrections, debt/credit and explicit confirmation; no destructive edits/deletes.
4. Coordinated spool -> Material Request wire migration only across all affected contracts together.
5. Full software regression/backup/restore.
6. Final two-board hardware E2E.

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
docs/PROJECT_HANDOFF/115_CLIENT_WEB_CRM_2026-08-26.md
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
Продолжаем CoilMaster. Source-of-truth только cmp-protocol-v1; main не использовать. Checkpoint 115 Client Web GREEN: CMP 32936343060 SUCCESS. Motor Web checkpoint 114 firmware build remains ESP32 32934092563 SUCCESS. Clients catalog/create/details закрыты; client card показывает motors through repair history, open/closed repairs, delivery state/date, charged/paid/debt/credit and append-only payment history; repairs page больше не создаёт клиента inline. Следующий блок: dedicated cash.html on checkpoint 112 APIs. costing.html оставить только costing/pricing. RUN_COMPLETED ничего автоматически не списывает; exact spool contract пока не удалять.
```
