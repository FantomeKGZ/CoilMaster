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
```

Latest evidence:

```text
CMP run 32857435377 / SUCCESS
ESP32 Build run 32857318798 / SUCCESS
```

## Current material architecture

Existing `MaterialLedger` is the authoritative generic warehouse item catalog.

Canonical request mapping:

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

`loadActiveMaterialState()` provides one authoritative fail-closed ACTIVE item scan with unit, stock, accounting price and currency.

Material Request movements:

```text
ISSUE | RETURN | CORRECTION
MANUAL_MATERIAL | RUN_WIRE
KG | L | PCS | M | M2
```

RUN_WIRE remains KG-only with exact source session/run provenance. No new runtime stock-mutation API exists yet; current exact-spool/writeoff flow remains authoritative.

## Next mandatory block

1. Append-only Material Request status history.
2. Authoritative lifecycle resolver:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

3. Backup/integrity coverage for status history.
4. Crash-safe transaction coordinator for explicit operator ISSUE/RETURN/CORRECTION coupling MaterialLedger stock mutation with durable request movement evidence.
5. Delivery store/API.
6. Payment/correction store/API.
7. Motor Web / Client Web.
8. Coordinated spool -> material-request wire migration.
9. Costing/cash integration.
10. Full regression/backup/restore and final two-board hardware E2E.

## Wire accounting target

```text
RUN_COMPLETED -> never auto-deducts
operator explicitly confirms warehouse ISSUE
material_request_id
source_session_id + source_run_id for run-linked wire
CU/AL + actual weight
```

Current `spool_id` requirements remain until the whole chain is migrated coherently.

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

Not complete. Full two-board hardware E2E remains mandatory after CRM/material/writeoff contracts stabilize.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/106_MATERIAL_CATALOG_ADAPTER_AND_LOOKUP_2026-08-25.md
docs/PROJECT_HANDOFF/105_MATERIAL_CATALOG_SERIALIZATION_FIX_2026-08-25.md
docs/PROJECT_HANDOFF/104_CRM_BACKUP_INTEGRITY_2026-08-25.md
docs/PROJECT_HANDOFF/103_MATERIAL_REQUEST_SCHEMA_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
this file
```

## Continuation prompt

```text
Продолжаем CoilMaster. Source-of-truth только cmp-protocol-v1; main не использовать. Прочитай AGENTS.md, 00, 95, 101, 106, 105, 104, 103, 06, 01 и 90. MaterialLedger catalog foundation и Material Request adapter GREEN. Следующий блок: append-only Material Request status history + resolver DRAFT->ISSUED->PRICED->CLOSED, затем backup/integrity и crash-safe explicit ISSUE/RETURN/CORRECTION coordinator. RUN_COMPLETED ничего автоматически не списывает; exact spool contract пока не удалять.
```
