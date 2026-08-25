# Активная работа и следующие шаги

Дата обновления: **2026-08-25**  
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
                     -> CASH/PAYMENTS
                     -> COMPLETED -> DELIVERED
```

## Phase A progress — GREEN

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
107 Material Request append-only lifecycle + backup/integrity
```

Latest lifecycle evidence:

```text
final head a960999b040afbdd7c48bbde08763e042408a2e8
CMP run 32860049965 / SUCCESS
ESP32 Build run 32860049946 / SUCCESS
```

Lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Status history:

```text
/data/workshop/material-request-status.ndjson
```

Backup/export includes status history. CRM deep audit validates legal transitions, monotonic transition IDs, request references and now accepts request movement unit `M2` consistently with the production movement store.

## Current NEXT

1. Build a crash-safe warehouse transaction coordinator for explicit operator:

```text
ISSUE
RETURN
CORRECTION
```

2. Couple physical `MaterialLedger` mutation and durable Material Request movement evidence without a two-append crash window.
3. Add durable pending/recovery marker for unfinished warehouse-request transactions.
4. Enforce request lifecycle gates around warehouse operations; no implicit status rewrite.
5. `RUN_COMPLETED` remains non-mutating.
6. After transaction foundation is GREEN, expose bounded runtime/Web API for request create/read/status/movements and explicit warehouse operations.
7. Then delivery event/store/API.
8. Then payment/correction store/API.
9. Motor Web / Client Web redesign.
10. Coordinated spool -> material-request wire migration only after all safety contracts are updated together.

## Warehouse/catalog contract

Existing `MaterialLedger` remains the authoritative generic catalog.

Canonical mapping:

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

Material Request movement units:

```text
KG | L | PCS | M | M2
```

`RUN_WIRE` remains KG-only and requires exact `source_session_id + source_run_id`, CU/AL and diameter.

## Wire migration rule

Current exact `spool_id` contract remains authoritative until coordinated migration across:

```text
job/writeoff
material-request movement
costing/finalization
backup/integrity/reports
Web/tests
```

Future run-linked wire issue remains manual and keeps exact session/run provenance.

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
- run-linked wire movement preserves exact run provenance;
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Documentation discipline

Synchronize after each meaningful block:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md when relevant
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
00_READ_FIRST.md when entrypoint/read order changes
```

Create numbered checkpoint with exact commits + CI evidence for each major store/API block.
