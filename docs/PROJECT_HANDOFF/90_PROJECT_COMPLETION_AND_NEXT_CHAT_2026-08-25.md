# CoilMaster — completion estimate and next-chat transfer

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Repo: `FantomeKGZ/CoilMaster`

## Stable baseline / source rule

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable pre-CRM
stable-2026-08-25-pre-crm-redesign -> same commit
```

Вся разработка после snapshot идёт только в `cmp-protocol-v1`. `main` не использовать как source и не двигать без нового согласованного stable checkpoint.

## Current architecture

Authoritative design:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

Current flow:

```text
CLIENT
-> MOTOR
-> REPAIR
-> immutable AS_RECEIVED
-> WORKING/STARTING winding versions/jobs
-> MATERIAL REQUEST
   -> WAREHOUSE ISSUE/RETURN/CORRECTION
   -> COSTING
-> CASH/PAYMENTS/BALANCE
-> repair completed
-> DELIVERED_TO_CLIENT
```

Warehouse = physical inventory. Cash = financial events. Material Request = bridge document owned by repair.

## Phase A status

SOFTWARE GREEN:

```text
97  Motor winding versions
98  Repair AS_RECEIVED store
99  Runtime/read API
100 Repair intake pending transaction
102 Transactional POST /api/repairs + recovery
103 Material Request identity + movement schema foundation
```

Transactional repair evidence:

```text
CMP #3182 / run 32851184680 / SUCCESS
ESP32 Build #1460 / run 32851184075 / SUCCESS
```

Material Request evidence:

```text
ESP32 Build / run 32851843400 / SUCCESS
CMP #3189 / run 32852061125 / SUCCESS
```

## Material Request current implementation

Stores:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
```

Requests preserve `repair_id + client_id + motor_id` and start in DRAFT.

Movements support:

```text
ISSUE | RETURN | CORRECTION
MANUAL_MATERIAL | RUN_WIRE
KG | L | PCS | M
integer quantity_milli_units
unit_cost_minor / cost_amount_minor / currency
```

RUN_WIRE requires exact `source_session_id + source_run_id`, CU/AL and KG. No Material Request mutation API or physical stock decrement is exposed yet. Old exact-spool/writeoff flow remains authoritative.

## Next mandatory block

Before Material Request becomes release-critical/runtime-writable:

1. add new CRM NDJSON files to backup/export coverage;
2. treat repair-intake pending/temp as recovery markers;
3. add fail-closed CRM persistence/cross-reference integrity audit;
4. verify ESP32 Build + CMP and record checkpoint;
5. implement generic warehouse item catalog + bounded unit/accounting cost contract;
6. implement Material Request status/API and explicit ISSUE/RETURN/CORRECTION;
7. delivery store/API;
8. payment/correction store/API;
9. Motor Web;
10. Client Web;
11. coordinated spool -> material-request wire migration;
12. costing/cash integration;
13. archive/navigation/analytics foundations;
14. full regression/backup/restore + two-board hardware E2E.

## Wire accounting target

```text
RUN_COMPLETED -> never auto-deducts
operator explicitly confirms warehouse ISSUE
material_request_id
source_session_id + source_run_id for run-linked wire
CU/AL + actual weight
```

Do not partially remove current `spool_id` requirements. Migration must cover job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests coherently.

## Web targets

Motor:
- separate `motor-new.html`;
- catalog-only `motors.html` based on Arduino archive UX;
- `motor-details.html` work card with versions, WORKING/STARTING and direct JOB send;
- repair/material-request links.

Client:
- separate `client-new.html`;
- catalog-only `clients.html`;
- `client-details.html` with motors, repairs, requests, payments/balance and delivery.

Cash:
- separate from warehouse;
- partial/multiple payments, corrections/refunds, debt/overpayment;
- navigate payment ↔ repair ↔ request ↔ warehouse.

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

Not complete. Full final two-board hardware E2E remains mandatory after CRM/material/writeoff contracts stabilize.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/103_MATERIAL_REQUEST_SCHEMA_2026-08-25.md
docs/PROJECT_HANDOFF/102_TRANSACTIONAL_REPAIR_INTAKE_INTEGRATION_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
this file
```

## Continuation prompt

```text
Продолжаем CoilMaster. Repo FantomeKGZ/CoilMaster, source-of-truth только cmp-protocol-v1; main не использовать и не двигать. Прочитай AGENTS.md, 00, 95, 101, 103, 102, 06, 01 и 90. Stable pre-CRM baseline 449570d... сохранён в main и stable-2026-08-25-pre-crm-redesign.

Transactional repair intake и Material Request schema foundation GREEN. Следующий обязательный блок: backup/export + fail-closed integrity для winding versions, AS_RECEIVED, material requests/movements и repair-intake recovery markers. После этого generic warehouse item catalog и Material Request status/API. RUN_COMPLETED ничего автоматически не списывает; exact spool contract пока не удалять.

После каждого meaningful block сразу обновляй 95/101/06/01/90 и при необходимости 00; крупные persistence/API блоки фиксируй numbered checkpoints с commits/CI evidence.
```
