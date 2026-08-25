# CoilMaster — completion estimate and next-chat transfer

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Repo: `FantomeKGZ/CoilMaster`

Этот checkpoint — authoritative transfer для продолжения проекта.

## Stable pre-CRM baseline

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable baseline
stable-2026-08-25-pre-crm-redesign -> same commit
```

После snapshot вся разработка только в `cmp-protocol-v1`; `main` не использовать как source и не двигать без нового согласованного stable checkpoint.

## Current active phase

Workshop Web/CRM redesign.

Authoritative design:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

Active queue: `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

Полный hardware acceptance ранее был начат, выявил B/operator-exit defect и ещё не завершён. После CRM/material/writeoff changes полный E2E требуется повторить.

## Current domain architecture

```text
CLIENT
-> physical MOTOR
-> REPAIR
-> immutable AS_RECEIVED snapshot
-> WORKING / STARTING winding versions/jobs
-> MATERIAL REQUEST
   -> WAREHOUSE ISSUE/RETURN/CORRECTION
   -> COSTING/material valuation
-> CASH/PAYMENTS/BALANCE
-> repair completion
-> DELIVERED_TO_CLIENT
-> read-only/archive history
```

Responsibilities:

```text
WAREHOUSE = physical inventory and material movements
MATERIAL REQUEST = repair-specific bridge document
COSTING = cost/charge calculation
CASH = financial events and client balance
```

## Implemented Phase A foundations

GREEN:

- checkpoint 97: motor winding versions;
- checkpoint 98: immutable repair AS_RECEIVED store;
- checkpoint 99: winding/snapshot read API;
- checkpoint 100: repair intake pending transaction foundation.

Transactional `POST /api/repairs` integrated commit:

```text
92523c7c6f4c8af8c71a63c4178a4b1e41953f19
```

Uses `RepairIntakeCoordinator` to prepare durable pending state, create repair, append/verify AS_RECEIVED and recover fail-closed after power loss. This integration still needs a normal connector-triggered ESP32 Build + CMP regression before being called GREEN.

## Newly approved Material Request architecture

Checkpoint 101 defines the next foundation.

Target request identity:

```text
material_request_id
repair_id
client_id
motor_id
status
```

Lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Target movement kinds:

```text
ISSUE | RETURN | CORRECTION
```

Materials include wire, varnish/lacquer, wedges/sticks, insulation, bearings and arbitrary warehouse items.

Issued history is not silently rewritten. Return/correction is append-only.

After CLOSED the request stays queryable via client/motor/repair history; no duplicate archive copy is required.

## Wire accounting migration

The approved move away from mandatory exact `spool_id` is now part of Material Request design.

Future contract:

```text
RUN_COMPLETED -> never auto-deducts
operator explicitly confirms wire ISSUE
material_request_id
exact source_session_id + source_run_id
CU/AL + actual consumed weight
```

Current production backend still requires exact spool identity. Do not partially remove it. Migration must update job/writeoff/request movement/costing/finalization/backup/integrity/reports/Web/tests coherently. `spool_id` may remain optional inventory metadata afterwards.

## Motor Web target

- catalog-only `motors.html` in Arduino archive style;
- separate `/desktop/motor-new.html`;
- `motor-details.html` as main work card;
- one physical motor = one `motor_id`;
- Al->Cu and later rewinds = winding versions;
- WORKING/STARTING + multi-conductor;
- direct JOB send from motor card without automatic physical START;
- repair/material-request history links.

## Client Web target

- catalog-only `clients.html`;
- `/desktop/client-new.html`;
- `/desktop/client-details.html`;
- links to motors, repairs, material requests, payments/balance and delivered dates;
- no permanent owner field in motor master.

## Costing / Cash target

Costing consumes confirmed warehouse movements and distinguishes:

```text
cost_amount = workshop cost
charge_amount = amount charged to client
```

Cash is append-only payment/correction/refund subsystem with partial/multiple payments, debt/overpayment and aggregated client balance.

`cash.html` must navigate payment -> repair -> material request -> warehouse and back.

## Delivery

Repair `CLOSED` and physical delivery are distinct. Delivery evidence append-only. Debt creates warning + explicit operator confirmation, not permanent hard block.

## Next implementation order

```text
1. Verify transactional POST /api/repairs with ESP32 Build + CMP regression.
2. Backup/integrity coverage for winding/snapshot/intake stores.
3. Material Request schema + movement store + warehouse item/unit contract.
4. Delivery event/store/API.
5. Payment/correction store/API.
6. Motor Web redesign.
7. Client Web redesign.
8. Coordinated spool -> material-request wire migration.
9. Costing/material-request integration.
10. cash.html/payment integration.
11. Archive/navigation/analytics foundations.
12. Software regression + backup/restore audit.
13. Full hardware E2E acceptance.
```

## Source/work rules

- source-of-truth only `cmp-protocol-v1`;
- fetch current content + blob SHA before each existing-file modification;
- check 404 before new file creation;
- never claim GREEN without actual evidence;
- update PROJECT_HANDOFF with each meaningful block.

## Safety invariants

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly drives SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never auto-deducts material;
- warehouse ISSUE requires explicit operator action;
- run-linked wire movements preserve exact session/run provenance;
- cancellation/operator abort preserves immutable history;
- restore operator-only, transactional, fail-closed;
- no automatic production deletion/truncation.

## Read order for next chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
this file
docs/PROJECT_HANDOFF/100_REPAIR_INTAKE_TRANSACTION_FOUNDATION_2026-08-25.md
docs/PROJECT_HANDOFF/99_CRM_WINDING_LOOKUP_API_2026-08-25.md
docs/PROJECT_HANDOFF/98_REPAIR_AS_RECEIVED_SNAPSHOT_2026-08-25.md
docs/PROJECT_HANDOFF/97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md
```

## Ready-to-paste continuation prompt

```text
Продолжаем CoilMaster. Repo FantomeKGZ/CoilMaster, source-of-truth только cmp-protocol-v1; main не использовать и не двигать. Прочитай AGENTS.md, 00_READ_FIRST, 95, 101, 06, 01 и 90. Stable pre-CRM baseline 449570d... сохранён в main и stable-2026-08-25-pre-crm-redesign.

Текущий блок: transactional POST /api/repairs уже integrated commit 92523c7c..., но ещё требует normal ESP32 Build + CMP regression. После GREEN добавить new stores в backup/integrity, затем начать Material Request schema: Склад отвечает за физические ISSUE/RETURN/CORRECTION, касса за деньги, Material Request связывает repair с warehouse/costing/cash. RUN_COMPLETED ничего автоматически не списывает. Для run-linked wire movement сохранять exact source_session_id + source_run_id. Exact spool contract пока не удалять до полной migration.

После каждого meaningful block сразу обновляй 95/101/06/01/90 и при необходимости 00, а крупные persistence/API блоки фиксируй numbered checkpoints с commits/CI evidence.
```
