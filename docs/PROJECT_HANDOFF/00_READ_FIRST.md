# CoilMaster — current project entrypoint

Дата обновления: **2026-08-25**  
Repo: `FantomeKGZ/CoilMaster`  
Source-of-truth: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Stable pre-CRM snapshot

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> same commit
stable-2026-08-25-pre-crm-redesign -> same commit
```

Вся новая разработка только в `cmp-protocol-v1`.

## Read order

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/103_MATERIAL_REQUEST_SCHEMA_2026-08-25.md
docs/PROJECT_HANDOFF/102_TRANSACTIONAL_REPAIR_INTAKE_INTEGRATION_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/100_REPAIR_INTAKE_TRANSACTION_FOUNDATION_2026-08-25.md
docs/PROJECT_HANDOFF/99_CRM_WINDING_LOOKUP_API_2026-08-25.md
docs/PROJECT_HANDOFF/98_REPAIR_AS_RECEIVED_SNAPSHOT_2026-08-25.md
docs/PROJECT_HANDOFF/97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md
```

Checkpoint 95 = authoritative CRM design.  
Checkpoint 101 = authoritative Warehouse ↔ Material Request ↔ Cash design.  
Checkpoint 103 = latest GREEN Material Request persistence foundation.  
Checkpoint 06 = active queue.  
Checkpoint 90 = transfer state.

## Current phase / target flow

```text
CLIENT -> MOTOR -> REPAIR -> AS_RECEIVED
                     -> WINDING VERSION/JOBS
                     -> MATERIAL REQUEST
                          -> WAREHOUSE ISSUE/RETURN/CORRECTION
                          -> COSTING
                     -> CASH/PAYMENTS
                     -> COMPLETED -> DELIVERED
```

Warehouse = physical inventory. Cash = money. Material Request = repair-specific bridge.

## Current GREEN implementation

```text
97  motor winding versions
98  repair AS_RECEIVED
99  runtime/read API
100 repair intake pending transaction
102 transactional POST /api/repairs
103 Material Request identity + movement schema
```

Latest Material Request verification:

```text
ESP32 Build run 32851843400 / SUCCESS
CMP #3189 run 32852061125 / SUCCESS
```

Material Request stores:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
```

Movement contract supports ISSUE/RETURN/CORRECTION and MANUAL_MATERIAL/RUN_WIRE. RUN_WIRE requires exact `source_session_id + source_run_id`, CU/AL and KG. No new runtime warehouse deduction API exists yet; old exact-spool flow remains authoritative.

## Immediate NEXT

Before new CRM stores become release-critical:

1. backup/export coverage for winding versions, AS_RECEIVED, material requests/movements;
2. repair-intake pending/temp as recovery markers;
3. fail-closed CRM persistence/cross-reference integrity audit;
4. verify ESP32 + CMP;
5. then generic warehouse item catalog and Material Request status/API.

## Wire migration rule

```text
RUN_COMPLETED never auto-deducts material.
Operator explicitly confirms warehouse ISSUE.
Run-linked wire movement preserves material_request_id + source_session_id + source_run_id + CU/AL + actual weight.
```

Current exact `spool_id` requirements remain until coordinated migration across job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests.

## Safety invariants

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts material;
- warehouse ISSUE requires explicit operator action;
- run-linked wire movement preserves exact run provenance;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production deletion/truncation.

## Working discipline

Before modifying an existing file: fetch exact `cmp-protocol-v1` content + current blob SHA. Before a new path: confirm 404. Never claim GREEN without actual CI/build evidence.

Synchronize 95/101/06/01/90 after meaningful blocks and update this file when entrypoint/read order changes. Major persistence/API work gets a numbered checkpoint with exact commits + CI evidence.
