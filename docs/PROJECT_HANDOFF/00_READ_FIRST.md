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
docs/PROJECT_HANDOFF/108_MATERIAL_REQUEST_WAREHOUSE_PENDING_TRANSACTION_2026-08-25.md
docs/PROJECT_HANDOFF/107_MATERIAL_REQUEST_LIFECYCLE_2026-08-25.md
docs/PROJECT_HANDOFF/106_MATERIAL_CATALOG_ADAPTER_AND_LOOKUP_2026-08-25.md
docs/PROJECT_HANDOFF/105_MATERIAL_CATALOG_SERIALIZATION_FIX_2026-08-25.md
docs/PROJECT_HANDOFF/104_CRM_BACKUP_INTEGRITY_2026-08-25.md
docs/PROJECT_HANDOFF/103_MATERIAL_REQUEST_SCHEMA_2026-08-25.md
docs/PROJECT_HANDOFF/102_TRANSACTIONAL_REPAIR_INTAKE_INTEGRATION_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

Latest GREEN foundation = checkpoint 108.

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
108 Material Request warehouse pending persistence + stable-backup guard
```

Checkpoint 108 verified:

```text
ESP32 Build run 32861148982 / SUCCESS
CMP permanent regression run 32861266055 / SUCCESS
backup guarded patch run 32861669436 / SUCCESS
```

## Current material contract

`MaterialLedger` is authoritative generic warehouse item catalog.

```text
KG->GRAM x1000
L->MILLILITRE x1000
PCS->PIECE x1
M->METRE x1
M2->SQUARE_METRE x1
```

Material Request lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Warehouse pending recovery markers:

```text
/data/workshop/material-request-warehouse.pending.json
/data/workshop/material-request-warehouse.pending.tmp
```

Both block stable backup while unresolved.

## Immediate NEXT

1. Implement crash-safe `MaterialRequestWarehouseCoordinator`.
2. Movement evidence first, physical MaterialLedger mutation second.
3. Use transaction-ref evidence for idempotent reboot recovery.
4. Explicit ISSUE/RETURN/CORRECTION only; no implicit lifecycle rewrite.
5. Then bounded runtime/Web APIs for material requests and warehouse operations.
6. `RUN_COMPLETED` remains non-mutating.

## Wire migration rule

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
