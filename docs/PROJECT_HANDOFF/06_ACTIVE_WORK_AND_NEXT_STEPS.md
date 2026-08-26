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
                     -> CASH/PAYMENTS/BALANCE
                     -> COMPLETED -> DELIVERED
```

## Backend foundation — GREEN

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
107 Material Request lifecycle + backup/integrity
108 Warehouse pending transaction persistence + stable-backup guard
109 Crash-safe Material Request warehouse coordinator
110 Material Request production runtime/Web API
111 Repair delivery store/API + backup/integrity
112 Cash payment/correction journal + repair/client balance API + backup/integrity
```

Backend verification:

```text
CMP 32928743465 / SUCCESS
ESP32 Build 32928706196 / SUCCESS
```

## Motor Web — Part A GREEN

Checkpoint: `113_MOTOR_WEB_CATALOG_AND_VERSIONED_CARD_2026-08-26.md`

Completed:

- desktop `motors.html` is catalog-only;
- separate `/desktop/motor-new.html` exists;
- catalog shows phases, WORKING, STARTING and conductor summary;
- latest version uses `/api/motors/winding/latest`;
- legacy `coil_program + repeat_target` remains explicit synthesized WORKING;
- `motor-details.html` shows current version and bounded version history;
- WORKING/STARTING roles and canonical multi-conductor strings are visible;
- repair history remains bounded and linked;
- Web/SSR physical START safety wording remains explicit.

Verification:

```text
CMP 32932380926 / SUCCESS
CMP 32932518963 / SUCCESS
```

## Current active queue — Web/CRM UI

### 1. Finish Motor Web

1. Add AS_RECEIVED / after-rewind context to the motor card using immutable repair snapshots.
2. Audit the existing linked-job path before adding direct WORKING/STARTING actions.
3. Role-aware send must reuse exact repair/motor/spool/snapshot safety and must never create a shortcut around linked-job preflight.
4. If no safe direct role handoff exists yet, extend the existing linked-job flow rather than POSTing a second job path.
5. After role-aware job flow regression is GREEN, Motor Web is closed.

### 2. Client Web

- `clients.html` catalog-only; remove creation form.
- Create `client-new.html`.
- Create `client-details.html?client_id=...`.
- Remove duplicated inline client creation from repairs page; leave link/button to client-new.
- Client card must show:
  - identity/contact;
  - motors brought through repair history;
  - open/closed repairs;
  - charges/payments/debt/credit;
  - payment history;
  - completed/delivered status and dates.
- Motor ownership is historical through repairs; do not make mutable `client_id` part of permanent physical motor identity.

### 3. Dedicated Cash UI

- Create `cash.html` using checkpoint 112 APIs.
- `costing.html` remains cost/price/margin, not payments.
- Main columns target:
  `Дата | Клиент | Двигатель | Ремонт | Начислено | Оплачено | Остаток | Статус`.
- Support payment, partial payments, correction, debt and credit display.
- No destructive payment edits.

### 4. Later coordinated wire migration

Do not partially remove exact `spool_id`. Migration must change together:

```text
job/writeoff
Material Request movement
costing/finalization
backup/integrity/reports
Web/tests
```

Target still:

```text
RUN_COMPLETED -> non-mutating
operator explicit warehouse ISSUE
material_request_id
source_session_id + source_run_id for RUN_WIRE
CU/AL + actual weight
```

### 5. Acceptance

- full Web/backend software regression;
- backup/export/restore integrity;
- then full two-board hardware E2E.

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
- cash events never mutate machine/warehouse state;
- delivery does not erase debt/history;
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Documentation discipline

Synchronize after each meaningful block: 95, 101 when relevant, 06, 01, 90; update 00 when read order changes. Create a numbered checkpoint with exact CI evidence for every major store/API/UI block.
