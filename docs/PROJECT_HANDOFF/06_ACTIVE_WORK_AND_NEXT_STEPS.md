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
```

Latest catalog foundation evidence:

```text
CMP run 32857435377 / SUCCESS
ESP32 Build run 32857318798 / SUCCESS
```

Existing `MaterialLedger` is authoritative generic warehouse catalog; no duplicate catalog is planned.

Canonical request mapping:

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

`MaterialLedger::loadActiveMaterialState()` now returns exact unit/stock/price/currency for one ACTIVE item through one fail-closed scan. Existing currency lookup reuses this path.

Material Request movements support:

```text
ISSUE | RETURN | CORRECTION
MANUAL_MATERIAL | RUN_WIRE
KG | L | PCS | M | M2
```

`RUN_WIRE` remains KG-only and requires exact `source_session_id + source_run_id`, CU/AL and diameter. Current exact-spool production flow is still authoritative.

## Current NEXT

1. Add append-only Material Request status history and authoritative resolver.
2. Enforce lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

3. Add fail-closed transition validation; no status rewrite of request identity.
4. Add backup/integrity coverage for status history before runtime use.
5. Build transactional explicit operator ISSUE/RETURN/CORRECTION coordinator coupling MaterialLedger stock mutation with durable Material Request movement evidence and crash recovery.
6. `RUN_COMPLETED` remains non-mutating.
7. Then delivery event/store/API.
8. Then payment/correction store/API.
9. Motor Web / Client Web redesign.
10. Coordinated spool -> material-request wire migration only after all safety contracts are updated together.

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
