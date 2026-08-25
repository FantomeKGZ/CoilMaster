# CoilMaster — current project entrypoint

Дата обновления: **2026-08-25**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Stable pre-CRM snapshot

```text
stable commit: 449570d47649d5f6336a31ee3eed491256e0fb1a
main -> same commit
stable-2026-08-25-pre-crm-redesign -> same commit
```

После snapshot разработка только в `cmp-protocol-v1`. `main` не двигать до следующего согласованного stable checkpoint.

## Что читать новому AI / coding agent

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/100_REPAIR_INTAKE_TRANSACTION_FOUNDATION_2026-08-25.md
docs/PROJECT_HANDOFF/99_CRM_WINDING_LOOKUP_API_2026-08-25.md
docs/PROJECT_HANDOFF/98_REPAIR_AS_RECEIVED_SNAPSHOT_2026-08-25.md
docs/PROJECT_HANDOFF/97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md
docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Checkpoint 95 — authoritative CRM design.  
Checkpoint 101 — authoritative Material Request / Warehouse ↔ Cash bridge.  
Checkpoint 06 — текущая очередь.  
Checkpoint 90 — authoritative transfer.

Старые checkpoints — history/evidence, не backlog.

Перед изменением existing file обязательно fetch exact current content из `cmp-protocol-v1` + current blob SHA. Для нового path сначала подтвердить 404/not found. Не объявлять CI/build/hardware GREEN без фактического результата.

## Current phase

Workshop Web/CRM redesign, Phase A foundations.

Текущий target flow:

```text
CLIENT
-> MOTOR
-> REPAIR
-> immutable AS_RECEIVED
-> WORKING / STARTING winding versions/jobs
-> MATERIAL REQUEST
   -> WAREHOUSE physical movements
   -> COSTING
-> CASH/PAYMENTS/BALANCE
-> repair completed
-> DELIVERED_TO_CLIENT
```

Главное новое разделение:

```text
СКЛАД = физический inventory и ISSUE/RETURN/CORRECTION
КАССА = денежные события и баланс
MATERIAL REQUEST = repair-specific документ, связывающий склад и стоимость/кассу
```

## Current implementation status

GREEN foundations:

- motor winding versions — checkpoint 97;
- repair AS_RECEIVED store — checkpoint 98;
- read/latest/history API — checkpoint 99;
- repair intake pending transaction foundation — checkpoint 100.

Transactional `POST /api/repairs` integrated commit:

```text
92523c7c6f4c8af8c71a63c4178a4b1e41953f19
```

Uses `RepairIntakeCoordinator`; ещё требует normal ESP32 Build + CMP regression до GREEN.

## Approved Material Request decisions

- owner = `repair_id`, plus exact `client_id + motor_id` provenance;
- target lifecycle `DRAFT -> ISSUED -> PRICED -> CLOSED`;
- warehouse movement kinds `ISSUE | RETURN | CORRECTION`;
- supports wire, varnish/lacquer, wedges/sticks, insulation, bearings and arbitrary warehouse items;
- after ISSUE no silent rewrite/delete; return/correction append-only;
- CLOSED request stays queryable in history, not copied to another archive;
- client/motor/repair/request/cash navigation is bidirectional.

## Wire accounting migration — important

Approved future flow is now part of Material Request architecture:

```text
RUN_COMPLETED never auto-deducts.
Operator confirms warehouse ISSUE manually.
Run-linked wire movement keeps:
material_request_id + source_session_id + source_run_id + CU/AL + actual weight.
```

Current backend/finalization still uses exact `spool_id`. Do not partially remove it. `spool_id` may become optional inventory metadata only after coordinated migration across job/writeoff/material-request/costing/finalization/backup/integrity/reports/Web/tests.

## Motor/Client/Cash target

- `motors.html` catalog-only in Arduino archive style;
- separate `motor-new.html`;
- `motor-details.html` as work card with versions, WORKING/STARTING, direct JOB send, repair/request history;
- `clients.html` catalog-only;
- separate `client-new.html` and `client-details.html`;
- client card includes motors, repairs, material requests, payments, balance, delivery dates;
- costing reads confirmed material request movements;
- cash stores payments/corrections/refunds only;
- repair CLOSED != delivered.

## Safety invariants — never weaken

- physical START only local;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never performs automatic material writeoff;
- warehouse ISSUE requires explicit operator action;
- run-linked wire movement preserves exact run provenance;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production deletion/truncation.

## Documentation rule

Keep synchronized after every meaningful implementation block:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md when relevant
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
this file when entrypoint/read order changes
```

For each major new store/API create a numbered checkpoint with exact commits + CI evidence.

## NDJSON rule

No premature DB migration. No destructive historical rewrite. Prefer bounded append-only version/event/movement stores. Every new production store must enter backup whitelist/integrity validation before becoming release-critical.
