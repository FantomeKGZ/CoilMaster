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
110 Material Request production runtime/Web API
```

Latest verification:

```text
CMP 32926200712 / SUCCESS
ESP32 Build 32926237400 / SUCCESS
```

Material Request production API:

```text
POST /api/material-requests
GET  /api/material-requests?repair_id=...
GET  /api/material-requests/item?material_request_id=...
GET  /api/material-requests/movements?material_request_id=...
GET  /api/material-requests/status?material_request_id=...
POST /api/material-requests/status
POST /api/material-requests/warehouse
```

Server derives client/motor from repair; warehouse actions require explicit confirmation, accept no client-supplied price and mutate stock only through the crash-safe coordinator.

## Current NEXT

1. Immutable delivery event/store/API.
2. Delivery is separate from repair completion and payment.
3. Delivery record keeps exact `repair_id + client_id + motor_id + delivered_at` and optional comment.
4. Only CLOSED repair may be delivered; zero balance is NOT a persistence requirement.
5. Include delivery journal in backup/export + CRM integrity.
6. Then append-only payment/correction journal/API and client/repair balance readers.
7. Motor Web redesign: separate `motor-new.html`, archive-style catalog, versioned WORKING/STARTING details and direct send without physical auto-start.
8. Client Web redesign: separate `client-new.html`, client card with motors, repairs, material requests, payments and delivery history.
9. Cash UI after payment/costing contracts stabilize.
10. Coordinated spool -> Material Request migration only after job/writeoff/finalization/backup/report/Web contracts are updated together.
11. Final software regression + backup/restore.
12. Full two-board hardware E2E.

## Warehouse/catalog contract

`MaterialLedger` remains authoritative generic catalog.

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

`RUN_WIRE` remains ISSUE/KG-only with exact `source_session_id + source_run_id`, CU/AL and diameter. `RUN_COMPLETED` remains non-mutating.

## Wire migration rule

Current exact `spool_id` contract remains authoritative until coordinated migration across job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests.

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

Synchronize after each meaningful block: 95, 101 when relevant, 06, 01, 90; update 00 when read order changes. Create a numbered checkpoint with exact CI evidence for every major store/API block.
