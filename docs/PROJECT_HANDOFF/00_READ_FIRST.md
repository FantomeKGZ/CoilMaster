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
docs/PROJECT_HANDOFF/110_MATERIAL_REQUEST_RUNTIME_WEB_API_2026-08-26.md
docs/PROJECT_HANDOFF/109_MATERIAL_REQUEST_WAREHOUSE_COORDINATOR_2026-08-26.md
docs/PROJECT_HANDOFF/108_MATERIAL_REQUEST_WAREHOUSE_PENDING_TRANSACTION_2026-08-25.md
docs/PROJECT_HANDOFF/107_MATERIAL_REQUEST_LIFECYCLE_2026-08-25.md
docs/PROJECT_HANDOFF/106_MATERIAL_CATALOG_ADAPTER_AND_LOOKUP_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

Latest GREEN foundation = checkpoint **110**.

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
```

Latest verified:

```text
CMP 32926200712 / SUCCESS
ESP32 Build 32926237400 / SUCCESS
```

## Immediate NEXT

1. Immutable repair delivery store/API (`repair CLOSED` is not the same as `DELIVERED`).
2. Delivery stores exact repair/client/motor/time and does not require zero balance.
3. Backup/integrity coverage for delivery.
4. Then payment/correction store/API and balances.
5. Then Motor/Client Web redesign.

## Material safety

`RUN_COMPLETED` never automatically deducts material. Warehouse mutation requires explicit operator action and goes through Material Request coordinator. Current exact `spool_id` production contract remains until coordinated migration across the whole chain.

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
