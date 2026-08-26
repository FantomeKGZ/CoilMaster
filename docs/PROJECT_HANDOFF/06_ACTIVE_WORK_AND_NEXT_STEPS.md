# Активная работа и следующие шаги

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Stable baseline

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable pre-CRM
stable-2026-08-25-pre-crm-redesign -> same commit
```

Все новые изменения только в `cmp-protocol-v1`.

## Authoritative design

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

Current domain flow:

```text
CLIENT -> MOTOR -> REPAIR -> AS_RECEIVED
                     -> WINDING VERSION/JOBS
                     -> MATERIAL REQUEST
                          -> WAREHOUSE ISSUE/RETURN/CORRECTION
                          -> COSTING
                     -> CASH/PAYMENTS
                     -> COMPLETED -> DELIVERED
```

## Phase A progress — GREEN

```text
97  Motor winding versions
98  Repair AS_RECEIVED
99  Runtime/read API
100 Repair intake pending transaction
102 Transactional POST /api/repairs
103 Material Request identity/movement schema
104 CRM backup/export + integrity
105 MaterialLedger catalog serialization fix
106 Material Request ↔ MaterialLedger unit adapter + active item lookup
107 Material Request append-only lifecycle + backup/integrity
108 Warehouse pending transaction persistence + stable-backup guard
109 Crash-safe Material Request warehouse coordinator
```

Checkpoint 109 evidence:

```text
CMP run 32925574132 / SUCCESS
CMP run 32925599254 / SUCCESS
```

Coordinator guarantees:

```text
operator confirmation
-> pending marker
-> immutable movement
-> MaterialLedger mutation
-> dual evidence verification
-> pending clear
```

Recovery:

```text
neither -> safe retry/no-op
movement only -> complete ledger
ledger only -> fail-closed
both -> clear pending
```

`CORRECTION` persists `ADD|REMOVE`; `RUN_WIRE` is ISSUE/KG-only with exact session/run provenance.

## Block 110 — Material Request runtime/Web API — IN PROGRESS

Implemented routes:

```text
POST /api/material-requests
GET  /api/material-requests?repair_id=...
GET  /api/material-requests/item?material_request_id=...
GET  /api/material-requests/movements?material_request_id=...
GET  /api/material-requests/status?material_request_id=...
POST /api/material-requests/status
POST /api/material-requests/warehouse
```

Rules:

- request creation derives `client_id + motor_id` server-side from `repair_id`;
- repair must exist and be OPEN to create a request;
- warehouse mutation requires `confirmed=true`;
- Web does not accept material pricing;
- warehouse mutation routes only through `MaterialRequestWarehouseCoordinator`;
- request stores/coordinator recover before routes are registered;
- lifecycle transitions remain explicit and separate;
- `RUN_COMPLETED` remains non-mutating.

Verification at last update:

```text
runtime bootstrap ESP32 Build 32926105448 / SUCCESS
API safety CMP 32926200712 / pending/running
integrated production ESP32 Build 32926237400 / pending/running
```

Do not mark block 110 GREEN until both current integrated checks succeed.

## Current NEXT

1. Close checkpoint 110 after integrated CMP + ESP32 GREEN.
2. Implement immutable delivery event/store/API:

```text
repair completed != delivered
DELIVERED records repair_id + client_id + motor_id + delivered_at
```

3. Implement append-only payment/correction journal/API:

```text
payment -> client_id + repair_id + amount + timestamp + method
correction -> separate event, never destructive edit
```

4. Add repair/client balance readers combining priced repair totals and payment journal.
5. Begin Motor Web redesign:
   - separate `motor-new.html`;
   - archive-style `motors.html`;
   - versioned WORKING/STARTING winding data in `motor-details.html`;
   - direct job send from motor card without physical auto-start.
6. Client Web redesign:
   - separate `client-new.html`;
   - `client-details.html` with motors, repairs, material requests, payments and delivery history.
7. Cash UI after payment/costing contracts are stable.
8. Coordinated spool -> material-request wire migration only after job/writeoff/finalization/backup/report/Web contracts are updated together.
9. Final full software regression + backup/restore.
10. Full two-board hardware E2E acceptance.

## Warehouse/catalog contract

Existing `MaterialLedger` remains authoritative generic catalog.

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

Current Material Request movement units:

```text
KG | L | PCS | M | M2
```

`RUN_WIRE` remains ISSUE/KG-only and requires exact `source_session_id + source_run_id`, CU/AL and diameter.

## Wire migration rule

Current exact `spool_id` contract remains authoritative until coordinated migration across:

```text
job/writeoff
material-request movement
costing/finalization
backup/integrity/reports
Web/tests
```

Future run-linked wire issue remains manual and keeps exact session/run provenance.

## Safety invariants

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
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Documentation discipline

Synchronize after each meaningful block:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md when relevant
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
00_READ_FIRST.md when entrypoint/read order changes
```

Create numbered checkpoint with exact commits + CI evidence for each major store/API block.
