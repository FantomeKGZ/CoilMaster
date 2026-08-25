# CoilMaster — Web/CRM motor, client, repair, warehouse and cash redesign

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **APPROVED DESIGN / PHASE A IMPLEMENTATION IN PROGRESS**

Этот checkpoint — authoritative design текущего Web/CRM этапа. История implementation blocks — numbered checkpoints 97+.

## Stable baseline / source rule

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable pre-CRM
stable-2026-08-25-pre-crm-redesign -> same commit
```

Вся новая работа только в `cmp-protocol-v1`. `main` не использовать как source и не двигать без отдельного stable checkpoint.

## Target domain flow

```text
CLIENT
-> physical MOTOR
-> REPAIR
-> immutable AS_RECEIVED
-> WORKING / STARTING winding versions/jobs
-> MATERIAL REQUEST
   -> WAREHOUSE ISSUE/RETURN/CORRECTION
   -> COSTING/material valuation
-> CASH/PAYMENTS/BALANCE
-> repair completed
-> DELIVERED_TO_CLIENT
-> archived/read-only history
```

Responsibilities:

```text
WAREHOUSE = physical inventory and material movements
MATERIAL REQUEST = repair-specific bridge document
COSTING = workshop cost + client charge calculation
CASH = financial events and client balance
```

Detailed material-request design: `101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md`.

## Motor model / Web target

One physical motor = one `motor_id`. Al->Cu and later rewinds are winding versions, not duplicate motors.

Each version supports separate WORKING and optional STARTING program/repeat data plus multi-conductor combinations such as `0.95 + 1.00` and `0.80 x 3`.

Target pages:

- `motors.html` catalog-only, based on Arduino archive UX;
- separate `/desktop/motor-new.html`;
- `motor-details.html` as working card with version/history/before-after data;
- direct WORKING/STARTING JOB send from the card, with physical START still local-only;
- repair/material-request history links.

## Client model / Web target

- `clients.html` catalog-only;
- separate `/desktop/client-new.html`;
- `/desktop/client-details.html` with motors, repairs, requests, payments, balance and delivery dates;
- remove duplicated inline create forms from repair flow and keep links to dedicated pages;
- motor master does not permanently own a `client_id`; ownership/history follows repairs.

## Repair / AS_RECEIVED / delivery

Every new repair captures immutable AS_RECEIVED evidence. Repair `CLOSED` and physical `DELIVERED_TO_CLIENT` remain different events.

Debt must warn and require explicit operator confirmation but does not permanently hard-block delivery.

## Material Request

Main owner = `repair_id`, with exact `client_id + motor_id` provenance.

Target lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Movements:

```text
ISSUE | RETURN | CORRECTION
```

Materials include wire, varnish/lacquer, wedges/sticks, insulation, bearings and arbitrary warehouse items. After ISSUE the original movement is never silently rewritten; return/correction is append-only.

A CLOSED request remains queryable through repair/client/motor history instead of being copied to another archive file.

## Wire accounting migration

Future target:

```text
RUN_COMPLETED
-> no automatic deduction
-> operator explicitly confirms warehouse ISSUE
-> material_request_id
-> exact source_session_id + source_run_id for run-linked wire
-> CU/AL + actual consumed weight
```

Current production backend/finalization still requires exact `spool_id`; do not partially remove it. Migration must update job/writeoff/request movement/costing/finalization/backup/integrity/reports/Web/tests as one coherent contract. `spool_id` may remain optional inventory metadata afterwards.

## Costing / Cash

Costing consumes confirmed warehouse movements and distinguishes:

```text
cost_amount = workshop cost
charge_amount = amount charged to client
```

Cash stores financial events only: charge/payment/correction/refund/balance. Partial/multiple payments, debt and overpayment are required. `cash.html` must navigate to repair/material-request/warehouse history and back.

## Phase A status

GREEN:

```text
97  Motor winding versions
98  Repair AS_RECEIVED persistence
99  Runtime/read lookup API
100 Repair intake pending transaction foundation
102 Transactional POST /api/repairs integration
103 Material Request identity + movement schema foundation
```

Transactional repair verification:

```text
CMP #3182 / run 32851184680 / SUCCESS
ESP32 Build #1460 / run 32851184075 / SUCCESS
```

Material Request persistence verification:

```text
ESP32 Build / run 32851843400 / SUCCESS / head a24c1e7e...
CMP #3189 / run 32852061125 / SUCCESS / head 10ce3c25...
```

Checkpoint 103 added append-only:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
```

The movement contract already supports `MANUAL_MATERIAL` and `RUN_WIRE`, bounded units, accounting cost snapshots and exact session/run provenance for run-linked wire. It is persistence foundation only: no new runtime warehouse deduction API yet, and the old spool/writeoff flow remains authoritative.

## Next implementation order

1. Add winding-version, AS_RECEIVED, material-request and request-movement files to backup/export coverage.
2. Treat repair-intake pending/temp as recovery markers, not normal backup business payload.
3. Add fail-closed CRM integrity audit/cross-reference validation.
4. Add generic warehouse item catalog + bounded unit/accounting-cost contract.
5. Add Material Request status/API and explicit stock ISSUE/RETURN/CORRECTION integration.
6. Add delivery event/store/API.
7. Add payment/correction store/API.
8. Motor Web redesign.
9. Client Web redesign.
10. Coordinated spool -> material-request wire migration.
11. Costing/material-request integration.
12. `cash.html` + payment integration.
13. Archive/navigation/analytics foundations.
14. Regression + backup/restore audit + full hardware E2E.

## Safety invariants — never weaken

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts material;
- physical warehouse ISSUE requires explicit operator action;
- run-linked wire ISSUE keeps exact `source_session_id + source_run_id`;
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Documentation discipline

After every meaningful implementation block update promptly:

```text
this file
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md when relevant
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
00_READ_FIRST.md when read order/entrypoint changes
```

Create numbered checkpoints for major new persistence/API blocks with exact commits and CI evidence.
