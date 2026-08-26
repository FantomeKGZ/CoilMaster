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
```

Latest verified block 110:

```text
CMP 32926200712 / SUCCESS
ESP32 Build 32926237400 / SUCCESS
```

Material Request production routes:

```text
POST /api/material-requests
GET  /api/material-requests?repair_id=...
GET  /api/material-requests/item?material_request_id=...
GET  /api/material-requests/movements?material_request_id=...
GET  /api/material-requests/status?material_request_id=...
POST /api/material-requests/status
POST /api/material-requests/warehouse
```

Server derives client/motor from repair; warehouse mutation requires explicit confirmation and coordinator path. Client-supplied material pricing is not accepted.

## Current work — delivery block

Immutable delivery journal/API is being added:

```text
/data/workshop/repair-deliveries.ndjson
GET/POST /api/repairs/delivery
```

Rules:

- delivery requires explicit confirmation;
- server derives exact `client_id + motor_id` from repair;
- repair must already be CLOSED;
- one final delivery per repair;
- payment/balance does not block persistence of delivery;
- delivery is exported in backup and validated read-only for repair identity, uniqueness and CLOSED state.

Do not call delivery checkpoint GREEN until current integrated CMP + ESP32 Build finish successfully.

## Next after delivery GREEN

1. Append-only payment/correction journal/API.
2. Repair/client balance readers.
3. Motor Web redesign with separate create page and versioned winding card.
4. Client Web redesign with separate create page and motors/repairs/material requests/payments/delivery history.
5. Cash UI and costing integration.
6. Coordinated spool -> Material Request wire migration.
7. Full software regression/backup/restore.
8. Final two-board hardware E2E.

## Wire accounting target

```text
RUN_COMPLETED -> never auto-deducts
operator explicitly confirms warehouse ISSUE
material_request_id
source_session_id + source_run_id for run-linked wire
CU/AL + actual weight
```

Current `spool_id` requirements remain until the whole chain is migrated coherently.

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
- run-linked wire movement preserves exact session/run provenance;
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
docs/PROJECT_HANDOFF/110_MATERIAL_REQUEST_RUNTIME_WEB_API_2026-08-26.md
docs/PROJECT_HANDOFF/109_MATERIAL_REQUEST_WAREHOUSE_COORDINATOR_2026-08-26.md
docs/PROJECT_HANDOFF/108_MATERIAL_REQUEST_WAREHOUSE_PENDING_TRANSACTION_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
this file
```

## Continuation prompt

```text
Продолжаем CoilMaster. Source-of-truth только cmp-protocol-v1; main не использовать. Checkpoint 110 Material Request production runtime/Web API GREEN: CMP 32926200712 SUCCESS, ESP32 32926237400 SUCCESS. Текущий блок: immutable repair delivery store/API + backup/integrity. Delivery разрешена только после CLOSED ремонта, не зависит от нулевого баланса и хранит exact repair/client/motor/time. После GREEN перейти к append-only payments/corrections and balance readers. RUN_COMPLETED ничего автоматически не списывает; exact spool contract пока не удалять.
```
