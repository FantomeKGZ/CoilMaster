# CoilMaster — current project entrypoint

Дата обновления: **2026-08-26**  
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
docs/PROJECT_HANDOFF/115_CLIENT_WEB_CRM_2026-08-26.md
docs/PROJECT_HANDOFF/114_MOTOR_WEB_ROLE_AWARE_LINKED_JOB_2026-08-26.md
docs/PROJECT_HANDOFF/113_MOTOR_WEB_CATALOG_AND_VERSIONED_CARD_2026-08-26.md
docs/PROJECT_HANDOFF/112_CASH_PAYMENT_LEDGER_AND_BALANCE_API_2026-08-26.md
docs/PROJECT_HANDOFF/111_REPAIR_DELIVERY_STORE_API_2026-08-26.md
docs/PROJECT_HANDOFF/110_MATERIAL_REQUEST_RUNTIME_WEB_API_2026-08-26.md
docs/PROJECT_HANDOFF/109_MATERIAL_REQUEST_WAREHOUSE_COORDINATOR_2026-08-26.md
docs/PROJECT_HANDOFF/108_MATERIAL_REQUEST_WAREHOUSE_PENDING_TRANSACTION_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

Latest GREEN foundation = checkpoint **115**.

## Current GREEN implementation

```text
97  motor winding versions
98  repair AS_RECEIVED
99  runtime/read API
100 repair intake pending transaction
102 transactional POST /api/repairs
103 Material Request identity + movement schema
104 CRM backup/export + fail-closed integrity
105 MaterialLedger serialization fix
106 Material Request ↔ MaterialLedger unit adapter + active item lookup
107 Material Request lifecycle + backup/integrity
108 warehouse pending transaction + backup guard
109 crash-safe warehouse coordinator
110 Material Request production runtime/Web API
111 immutable repair delivery store/API + backup/integrity
112 append-only cash/payment journal + repair/client balance API + backup/integrity
113 Motor Web catalog-only + separate create page + versioned motor card
114 immutable AS_RECEIVED comparison + role-aware linked WORKING/STARTING job flow
115 Client Web catalog-only + dedicated create + read-only CRM client card
```

Latest verified:

```text
checkpoint 115 CMP         32936343060 / SUCCESS
checkpoint 114 ESP32 Build 32934092563 / SUCCESS
```

Checkpoint 115 changes only Web/host regression files. Latest firmware-source build evidence remains checkpoint 114 ESP32 `32934092563` on production-source commit `da5d7271...`.

## Immediate NEXT

1. Dedicated `cash.html` using checkpoint 112 APIs; `costing.html` remains costing/pricing/margin, not payments.
2. Cash UI must support repair/client balances, full/partial/multiple payments and append-only corrections; no destructive payment edit/delete.
3. Coordinated spool -> Material Request wire migration only as one coherent backend/Web/test change.
4. Full Web/backend regression + backup/restore, then two-board hardware E2E.

## Client / motor identity boundary

Clients and motors stay independent domain identities. A physical motor does not gain mutable permanent `client_id`; client↔motor relation is historical through repair records. Client card reads motors via each repair's exact `motor_id`.

## Motor linked-job safety

Motor role buttons only navigate to the existing linked-job preparation page. Latest winding version is authoritative for exact role/program/repeat; legacy fallback is WORKING-only. STARTING without an explicit versioned STARTING role fails closed. Server rechecks exact role/program/repeat and current exact spool selection before immutable job persistence/UART. Physical START remains local-only.

## Material / cash safety

`RUN_COMPLETED` never automatically deducts material. Warehouse mutation requires explicit operator action through Material Request coordinator. Cash events are append-only and never trigger machine or warehouse actions. Delivery remains independent of zero balance. Current exact `spool_id` production contract remains until coordinated migration across the whole chain.

## General safety invariants

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production deletion/truncation.

## Working discipline

Before modifying an existing file: fetch exact `cmp-protocol-v1` content + current blob SHA. Before a new path: confirm 404. Never claim GREEN without actual CI/build evidence.
