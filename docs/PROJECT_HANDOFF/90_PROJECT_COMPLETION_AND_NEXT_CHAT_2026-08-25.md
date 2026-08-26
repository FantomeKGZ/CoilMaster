# CoilMaster — completion estimate and next-chat transfer

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**  
Repo: `FantomeKGZ/CoilMaster`

## Stable baseline / source rule

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable pre-CRM
stable-2026-08-25-pre-crm-redesign -> same commit
```

Вся разработка после snapshot идёт только в `cmp-protocol-v1`. `main` не использовать как source.

## Current architecture

```text
CLIENT -> MOTOR -> REPAIR -> immutable AS_RECEIVED
                     -> WORKING/STARTING versions/jobs
                     -> MATERIAL REQUEST
                          -> WAREHOUSE ISSUE/RETURN/CORRECTION
                          -> COSTING
                     -> CASH/PAYMENTS/BALANCE
                     -> COMPLETED -> DELIVERED
```

Authoritative design:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

## Software GREEN checkpoints

```text
97  Motor winding versions
98  Repair AS_RECEIVED
99  Runtime/read API
100 Repair intake pending transaction
102 Transactional POST /api/repairs
103 Material Request schema foundation
104 CRM backup/export + integrity
105 MaterialLedger serialization fix
106 Material Request ↔ MaterialLedger unit adapter + active item lookup
107 Material Request lifecycle + backup/integrity
108 Material Request warehouse pending + backup guard
109 Crash-safe warehouse coordinator
110 Material Request production runtime/Web API
111 Immutable repair delivery store/API + backup/integrity
112 Append-only cash payments/corrections + repair/client balance API + backup/integrity
113 Motor Web catalog-only + separate create + versioned motor card
114 Motor AS_RECEIVED comparison + role-aware linked WORKING/STARTING job flow
115 Client Web catalog-only + dedicated create + read-only CRM card
116 Dedicated Cash Web UI + append-only writes + exact BigInt minor units + navigation
```

Latest verified:

```text
CMP Protocol Tests 32938179528 / SUCCESS
ESP32 Build         32936718747 / SUCCESS
```

## Web/CRM status — GREEN

Motor, Client and Cash Web blocks are closed. `cash.html` is separate from `costing.html`; payment history is append-only and never controls machine, SSR, warehouse or delivery. Client↔motor relation stays historical through repair identity.

## Current mandatory work — coordinated wire accounting migration

Do not partially remove current exact `spool_id` requirements.

First map every current owner:

```text
job preparation / JobSpoolSelection
manual writeoff API/UI
warehouse/material mutation
source_session_id + source_run_id provenance
repair costing/material usage
finalization preflight
backup/export/integrity
reports
Web/tests
```

Then design and implement one coherent compatibility transition to:

```text
RUN_COMPLETED -> never auto-deducts
operator explicitly confirms warehouse ISSUE
material_request_id
source_session_id + source_run_id for RUN_WIRE
CU/AL + actual consumed weight
```

Material Request `RUN_WIRE` remains ISSUE/KG-only. New linked writeoff must never silently omit provenance or gain legacy permission to skip an already selected spool before the coordinated transition is complete.

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
- cash events never control machine/warehouse state;
- payment balance does not rewrite delivery/run/material evidence;
- cancellation/operator abort preserves immutable history;
- restore operator-only, transactional, fail-closed;
- no automatic production deletion/truncation.

## Hardware acceptance

Not complete. Full two-board hardware E2E remains mandatory after CRM/material/writeoff contracts stabilize.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/116_CASH_WEB_UI_2026-08-26.md
docs/PROJECT_HANDOFF/115_CLIENT_WEB_CRM_2026-08-26.md
docs/PROJECT_HANDOFF/114_MOTOR_WEB_ROLE_AWARE_LINKED_JOB_2026-08-26.md
docs/PROJECT_HANDOFF/112_CASH_PAYMENT_LEDGER_AND_BALANCE_API_2026-08-26.md
docs/PROJECT_HANDOFF/111_REPAIR_DELIVERY_STORE_API_2026-08-26.md
docs/PROJECT_HANDOFF/110_MATERIAL_REQUEST_RUNTIME_WEB_API_2026-08-26.md
docs/PROJECT_HANDOFF/109_MATERIAL_REQUEST_WAREHOUSE_COORDINATOR_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
this file
```

## Continuation prompt

```text
Продолжаем CoilMaster. Source-of-truth только cmp-protocol-v1; main не использовать. Checkpoint 116 Cash Web GREEN: CMP 32938179528 SUCCESS, ESP32 32936718747 SUCCESS. Motor/Client/Cash Web закрыты. Следующий блок — coordinated exact-spool -> Material Request RUN_WIRE accounting migration. Сначала составить forensic owner map всех current spool/writeoff/finalization/backup/report/Web contracts; ничего не ослаблять частично. RUN_COMPLETED ничего автоматически не списывает; physical START local-only; Arduino owns SSR.
```
