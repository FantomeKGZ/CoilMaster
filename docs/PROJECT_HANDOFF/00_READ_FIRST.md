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

Latest GREEN foundation = checkpoint 107.

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
```

Latest verified:

```text
head a960999b040afbdd7c48bbde08763e042408a2e8
CMP run 32860049965 / SUCCESS
ESP32 Build run 32860049946 / SUCCESS
```

## Current material contract

`MaterialLedger` is the authoritative generic warehouse item catalog.

```text
KG->GRAM x1000
L->MILLILITRE x1000
PCS->PIECE x1
M->METRE x1
M2->SQUARE_METRE x1
```

Material Request lifecycle is append-only:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Material Request movements support `ISSUE/RETURN/CORRECTION`, `MANUAL_MATERIAL/RUN_WIRE`, and `KG/L/PCS/M/M2`. `RUN_WIRE` remains KG-only with exact session/run provenance.

## Immediate NEXT

1. Crash-safe explicit operator ISSUE/RETURN/CORRECTION transaction coordinator.
2. Couple physical MaterialLedger mutation with durable request movement evidence via pending/recovery marker.
3. Enforce lifecycle gates and fail closed on ambiguous recovery.
4. Then bounded runtime/Web APIs for material requests and warehouse operations.
5. `RUN_COMPLETED` remains non-mutating.

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
