# CoilMaster — completion estimate and next-chat transfer

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Repo: `FantomeKGZ/CoilMaster`

## Stable baseline / source rule

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable pre-CRM
stable-2026-08-25-pre-crm-redesign -> same commit
```

Вся разработка после snapshot идёт только в `cmp-protocol-v1`. `main` не использовать как source и не двигать без нового согласованного stable checkpoint.

## Current architecture

Authoritative design:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

Current flow:

```text
CLIENT
-> MOTOR
-> REPAIR
-> immutable AS_RECEIVED
-> WORKING/STARTING winding versions/jobs
-> MATERIAL REQUEST
   -> WAREHOUSE ISSUE/RETURN/CORRECTION
   -> COSTING
-> CASH/PAYMENTS/BALANCE
-> repair completed
-> DELIVERED_TO_CLIENT
```

Warehouse = physical inventory. Cash = financial events. Material Request = bridge document owned by repair.

## Phase A status

SOFTWARE GREEN:

```text
97  Motor winding versions
98  Repair AS_RECEIVED store
99  Runtime/read API
100 Repair intake pending transaction
102 Transactional POST /api/repairs + recovery
103 Material Request identity + movement schema foundation
104 CRM backup/export + fail-closed integrity
```

Latest backup/integrity evidence:

```text
CMP run 32855540935 / SUCCESS
ESP32 Build run 32855541246 / SUCCESS
```

New CRM journals are in backup/export and `CM_CrmPersistenceIntegrityAudit` validates them read-only/fail-closed. Repair-intake pending/temp are recovery markers that block a stable backup snapshot.

## Material Request current implementation

Stores:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
```

Requests preserve `repair_id + client_id + motor_id` and start in DRAFT.

Movements support:

```text
ISSUE | RETURN | CORRECTION
MANUAL_MATERIAL | RUN_WIRE
KG | L | PCS | M
integer quantity_milli_units
unit_cost_minor / cost_amount_minor / currency
```

RUN_WIRE requires exact `source_session_id + source_run_id`, CU/AL and KG. No Material Request mutation API or physical stock decrement is exposed yet. Old exact-spool/writeoff flow remains authoritative.

## Next mandatory block

1. Reuse existing `MaterialLedger` as generic warehouse item catalog rather than creating a duplicate catalog.
2. Fix and regression-protect `MaterialLedger::addMaterial()` unit JSON serialization.
3. Define canonical unit mapping/accounting-cost semantics between the ledger and Material Request movements.
4. Expose warehouse item lookup/state required by request mutations.
5. Implement Material Request lifecycle `DRAFT -> ISSUED -> PRICED -> CLOSED`.
6. Add explicit operator ISSUE/RETURN/CORRECTION APIs; `RUN_COMPLETED` remains non-mutating.
7. Delivery store/API.
8. Payment/correction store/API.
9. Motor Web.
10. Client Web.
11. Coordinated spool -> material-request wire migration.
12. Costing/cash integration.
13. Archive/navigation/analytics foundations.
14. Full regression/backup/restore + two-board hardware E2E.

## Wire accounting target

```text
RUN_COMPLETED -> never auto-deducts
operator explicitly confirms warehouse ISSUE
material_request_id
source_session_id + source_run_id for run-linked wire
CU/AL + actual weight
```

Do not partially remove current `spool_id` requirements. Migration must cover job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests coherently.

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
- run-linked wire movement preserves exact session/run provenance;
- cancellation/operator abort preserves immutable history;
- restore operator-only, transactional, fail-closed;
- no automatic production deletion/truncation.

## Hardware acceptance

Not complete. Full final two-board hardware E2E remains mandatory after CRM/material/writeoff contracts stabilize.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/104_CRM_BACKUP_INTEGRITY_2026-08-25.md
docs/PROJECT_HANDOFF/103_MATERIAL_REQUEST_SCHEMA_2026-08-25.md
docs/PROJECT_HANDOFF/102_TRANSACTIONAL_REPAIR_INTAKE_INTEGRATION_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
this file
```

## Continuation prompt

```text
Продолжаем CoilMaster. Repo FantomeKGZ/CoilMaster, source-of-truth только cmp-protocol-v1; main не использовать и не двигать. Прочитай AGENTS.md, 00, 95, 101, 104, 103, 102, 06, 01 и 90. Stable pre-CRM baseline 449570d... сохранён в main и stable-2026-08-25-pre-crm-redesign.

Transactional repair intake, Material Request schema и CRM backup/integrity GREEN. Следующий блок: reuse существующего MaterialLedger как generic warehouse catalog. Сначала исправить unit JSON serialization в addMaterial и добавить regression, затем canonical unit mapping/lookup, Material Request status/API и explicit ISSUE/RETURN/CORRECTION. RUN_COMPLETED ничего автоматически не списывает; exact spool contract пока не удалять.
```
