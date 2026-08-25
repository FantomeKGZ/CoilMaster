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
108 Warehouse pending transaction persistence + stable-backup guard
```

Checkpoint 108 evidence:

```text
ESP32 Build run 32861148982 / SUCCESS
CMP run 32861149158 / SUCCESS
CMP permanent regression run 32861266055 / SUCCESS
backup guarded patch run 32861669436 / SUCCESS
```

Pending transaction paths:

```text
/data/workshop/material-request-warehouse.pending.json
/data/workshop/material-request-warehouse.pending.tmp
```

Both now block stable backup while transaction recovery is unresolved.

## Current NEXT

1. Implement crash-safe `MaterialRequestWarehouseCoordinator`.
2. Use movement-first + ledger-second ordering with durable `transaction_ref` evidence.
3. Recovery matrix must distinguish:

```text
neither side committed -> safe no-op/retry
movement only -> complete physical ledger mutation
ledger only -> impossible ordering / fail-closed
both committed -> clear pending
```

4. Support explicit operator `ISSUE`, `RETURN`, `CORRECTION` (`ADD|REMOVE`).
5. Lifecycle transitions stay explicit; warehouse coordinator must not silently change DRAFT/ISSUED/PRICED/CLOSED.
6. `RUN_COMPLETED` remains non-mutating.
7. After coordinator GREEN, expose bounded runtime/Web APIs for request create/read/status/movements and warehouse actions.
8. Then delivery event/store/API.
9. Then payment/correction store/API.
10. Motor Web / Client Web redesign.
11. Coordinated spool -> material-request wire migration only after all safety contracts are updated together.

## Warehouse/catalog contract

Existing `MaterialLedger` remains authoritative generic catalog.

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

`RUN_WIRE` remains ISSUE/KG-only and requires exact `source_session_id + source_run_id`, CU/AL and diameter.

## Wire migration rule

Current exact `spool_id` contract remains authoritative until coordinated migration across job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests.

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
