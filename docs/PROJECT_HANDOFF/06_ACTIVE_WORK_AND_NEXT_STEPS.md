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

## GREEN foundation through checkpoint 116

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
115 Client Web catalog-only + dedicated create + read-only CRM card
116 Dedicated Cash Web UI + append-only payments/corrections + BigInt money rendering + navigation
```

Latest verification:

```text
CMP 32938179528 / SUCCESS
ESP32 Build 32936718747 / SUCCESS
```

## Motor Web — CLOSED SOFTWARE GREEN

Checkpoint: `114_MOTOR_WEB_ROLE_AWARE_LINKED_JOB_2026-08-26.md`.

Exact role/program/repeat/spool remains server-owned; physical START stays local-only.

## Client Web — CLOSED SOFTWARE GREEN

Checkpoint: `115_CLIENT_WEB_CRM_2026-08-26.md`.

Client catalog/create/details are separated; motors are resolved historically through repairs; client card is read-only for cash/delivery/machine/material state.

## Cash Web — CLOSED SOFTWARE GREEN

Checkpoint: `116_CASH_WEB_UI_2026-08-26.md`.

Completed:

- dedicated `/desktop/cash.html`;
- exact repair context and bounded repair/payment reads;
- charged / paid / debt / credit;
- PAYMENT = append-only ADD;
- CORRECTION = append-only ADD/SUBTRACT;
- full/partial/multiple payment model;
- explicit confirmation for every write;
- no PUT/PATCH/DELETE;
- correction provenance via `corrects_event_id`;
- post-CLOSED/post-DELIVERED payments remain allowed;
- delivery remains independent from debt;
- exact integer minor-unit input/rendering using `BigInt`;
- main navigation and client-scoped cash navigation;
- costing remains separate and never owns cash mutation.

## Current active queue — coordinated wire accounting migration

### 1. Forensic owner map first

Before any runtime change, map all current owners of exact spool/run writeoff:

```text
job preparation + immutable spool selection
manual wire writeoff API/UI
warehouse spool / MaterialLedger mutation
run provenance source_session_id + source_run_id
repair costing/material usage
finalization preflight
backup/export/integrity
reports
Web regressions
```

Do not change any one owner in isolation.

### 2. Target contract

Future run-linked wire accounting target:

```text
RUN_COMPLETED -> non-mutating
operator explicit warehouse ISSUE
material_request_id
source_session_id + source_run_id
CU/AL
actual consumed weight
manual confirmation
```

Material Request `RUN_WIRE` movement remains ISSUE/KG-only.

### 3. Compatibility rule

Current exact `spool_id` contract stays authoritative until one coherent migration covers all affected owners, persistence/recovery, Web and tests. No partial relaxation, no legacy omission authorization for new linked writeoff.

### 4. Acceptance

- targeted regression for each touched safety boundary;
- full Web/backend software regression;
- backup/export/restore integrity;
- then final two-board hardware E2E.

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

Synchronize after each meaningful block: 95, 101 when relevant, 06, 01, 90; update 00 when read order changes. Create a numbered checkpoint with exact CI evidence for every major persistence/API/UI block.
