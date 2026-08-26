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
docs/PROJECT_HANDOFF/116_CASH_WEB_UI_2026-08-26.md
docs/PROJECT_HANDOFF/115_CLIENT_WEB_CRM_2026-08-26.md
docs/PROJECT_HANDOFF/114_MOTOR_WEB_ROLE_AWARE_LINKED_JOB_2026-08-26.md
docs/PROJECT_HANDOFF/112_CASH_PAYMENT_LEDGER_AND_BALANCE_API_2026-08-26.md
docs/PROJECT_HANDOFF/111_REPAIR_DELIVERY_STORE_API_2026-08-26.md
docs/PROJECT_HANDOFF/110_MATERIAL_REQUEST_RUNTIME_WEB_API_2026-08-26.md
docs/PROJECT_HANDOFF/109_MATERIAL_REQUEST_WAREHOUSE_COORDINATOR_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

Latest GREEN foundation = checkpoint **116**.

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
116 dedicated append-only Cash Web UI + exact BigInt minor-unit rendering + navigation
```

Latest verified:

```text
checkpoint 116 CMP         32938179528 / SUCCESS
checkpoint 116 ESP32 Build 32936718747 / SUCCESS
```

## Immediate NEXT

1. Map every current exact-spool owner before changing runtime behavior: job preparation/spool selection, manual writeoff, finalization/costing, reports, backup/integrity, Web and regressions.
2. Design one compatibility transition from exact `spool_id` run writeoff toward Material Request-owned `RUN_WIRE` ISSUE using `material_request_id + source_session_id + source_run_id + CU/AL + actual weight`.
3. Do not partially remove or bypass the current exact-spool contract; no migration commit until all owners and crash/recovery boundaries are accounted for.
4. After coherent implementation, run full Web/backend regression + backup/restore, then final two-board hardware E2E.

## Client / motor identity boundary

Clients and motors stay independent domain identities. A physical motor does not gain mutable permanent `client_id`; client↔motor relation is historical through repair records.

## Cash boundary

Cash is append-only and separate from costing, delivery, warehouse and machine state. `costing.html` owns cost/price/margin; `/desktop/cash.html` owns payment/correction UI. Payment never starts a machine, controls SSR, mutates Material Request/warehouse, or blocks delivery.

## Motor linked-job safety

Motor role buttons only navigate to the existing linked-job preparation page. Latest winding version is authoritative for exact role/program/repeat; legacy fallback is WORKING-only. Server rechecks exact role/program/repeat and current exact spool selection before immutable job persistence/UART. Physical START remains local-only.

## Material safety

`RUN_COMPLETED` never automatically deducts material. Warehouse mutation requires explicit operator action through Material Request coordinator. Current exact `spool_id` production contract remains authoritative until coordinated migration across the whole chain.

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
