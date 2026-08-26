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

## GREEN foundation through checkpoint 114

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
113 Motor Web catalog + separate create + versioned card
114 AS_RECEIVED comparison + role-aware linked WORKING/STARTING job flow
```

Latest verification:

```text
CMP 32934323481 / SUCCESS
ESP32 Build 32934092563 / SUCCESS
```

## Motor Web — CLOSED SOFTWARE GREEN

Checkpoint: `114_MOTOR_WEB_ROLE_AWARE_LINKED_JOB_2026-08-26.md`

Completed:

- desktop `motors.html` catalog-only;
- separate `/desktop/motor-new.html`;
- current and historical versioned WORKING/STARTING data;
- multi-conductor display;
- immutable `AS_RECEIVED` vs after-repair comparison;
- safe OPEN-repair links to existing linked-job page;
- `?role=working|starting` support;
- latest winding version authoritative for exact program + repeat target;
- legacy fallback WORKING-only;
- STARTING absence fails closed;
- program and repeat target readonly in linked UI;
- server revalidates exact role/program/repeat before persistence/UART;
- exact spool selection remains authoritative;
- no Web physical START / no direct SSR control.

## Current active queue — Client Web

### 1. Client catalog/create/card

- `clients.html` catalog-only; remove creation form.
- Create `client-new.html`.
- Create `client-details.html?client_id=...`.
- Remove duplicated inline client creation from repairs page; leave link/button to `client-new.html`.
- Client card must show:
  - identity/contact;
  - motors brought through repair history;
  - open/closed repairs;
  - material-request links where relevant;
  - charged / paid / debt / credit;
  - payment history;
  - completed/delivered status and dates.
- Motor ownership is historical through repairs; never add mutable `client_id` to permanent physical motor identity.

### 2. Dedicated Cash UI

- Create `cash.html` using checkpoint 112 APIs.
- `costing.html` remains cost/price/margin, not payments.
- Main columns target:
  `Дата | Клиент | Двигатель | Ремонт | Начислено | Оплачено | Остаток | Статус`.
- Support full/partial/multiple payments, correction, debt and credit display.
- No destructive payment edits.

### 3. Later coordinated wire migration

Do not partially remove exact `spool_id`. Migration must change together:

```text
job/writeoff
Material Request movement
costing/finalization
backup/integrity/reports
Web/tests
```

Target remains:

```text
RUN_COMPLETED -> non-mutating
operator explicit warehouse ISSUE
material_request_id
source_session_id + source_run_id for RUN_WIRE
CU/AL + actual weight
```

### 4. Acceptance

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
