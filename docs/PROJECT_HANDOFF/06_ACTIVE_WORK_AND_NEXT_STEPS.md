# Активная работа и следующие шаги

Дата обновления: **2026-08-25**  
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

## Phase A progress

GREEN:

```text
A1  Motor winding versions                    checkpoint 97
A2  Repair AS_RECEIVED                        checkpoint 98
A3  Runtime/read API                          checkpoint 99
A4  Repair intake pending transaction         checkpoint 100
A5  Transactional POST /api/repairs           checkpoint 102
A6  Material Request identity/movement schema checkpoint 103
```

Transactional repair evidence:

```text
CMP #3182 / run 32851184680 / SUCCESS
ESP32 Build #1460 / run 32851184075 / SUCCESS
```

Material Request schema evidence:

```text
ESP32 Build / run 32851843400 / SUCCESS
CMP #3189 / run 32852061125 / SUCCESS
```

Material Request stores now exist as persistence foundation:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
```

Movements support:

```text
ISSUE | RETURN | CORRECTION
MANUAL_MATERIAL | RUN_WIRE
KG | L | PCS | M
```

`RUN_WIRE` requires exact `source_session_id + source_run_id`, CU/AL and KG. This does not yet replace the existing exact-spool production flow.

## Current NEXT — mandatory before runtime Material Request mutations

1. Add new CRM files to backup/export whitelist:
   - motor winding versions;
   - AS_RECEIVED snapshots;
   - material requests;
   - material request movements.
2. Treat `/data/workshop/repair-intake.pending.json` and `.tmp` as recovery markers that block a stable backup snapshot.
3. Add fail-closed CRM persistence integrity audit and cross-reference validation.
4. Verify ESP32 Build + CMP and record a numbered checkpoint.
5. Implement generic warehouse item catalog with bounded units/accounting cost.
6. Implement Material Request status transitions `DRAFT -> ISSUED -> PRICED -> CLOSED`.
7. Expose explicit operator APIs for ISSUE/RETURN/CORRECTION; no automatic RUN_COMPLETED writeoff.
8. Add delivery event/store/API.
9. Add payment/correction store/API.

## Then Web phases

Motor:
- `/desktop/motor-new.html`;
- catalog-only `motors.html` based on Arduino archive UX;
- expanded `motor-details.html`;
- direct WORKING/STARTING JOB send without automatic physical START;
- repair/request history.

Client:
- `/desktop/client-new.html`;
- catalog-only `clients.html`;
- `/desktop/client-details.html`;
- motors/repairs/requests/payments/balance/delivery history.

## Wire migration rule

Current exact `spool_id` contract remains authoritative until a coordinated migration updates all of:

```text
job/writeoff
material-request movement
costing/finalization
backup/integrity/reports
Web/tests
```

Future run-linked wire issue:

```text
material_request_id
source_session_id + source_run_id
CU/AL
actual consumed weight
manual warehouse ISSUE confirmation
```

`RUN_COMPLETED` never deducts material automatically.

## Cash rule

Warehouse = physical stock. Cash = money. Costing reads confirmed material movements and can distinguish `cost_amount` and `charge_amount`. Cash stores payment/correction/refund events and balance.

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
