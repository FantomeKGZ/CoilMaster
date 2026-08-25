# Текущее состояние CoilMaster

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

## Source of truth / stable baseline

Working source-of-truth only `cmp-protocol-v1`. `main` не использовать как source.

```text
stable pre-CRM: 449570d47649d5f6336a31ee3eed491256e0fb1a
main -> same commit
stable-2026-08-25-pre-crm-redesign -> same commit
```

## Current phase

Workshop Web/CRM redesign.

Authoritative design:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

Target flow:

```text
CLIENT -> MOTOR -> REPAIR -> AS_RECEIVED
                     -> WINDING VERSION/JOBS
                     -> MATERIAL REQUEST
                          -> WAREHOUSE physical movements
                          -> COSTING
                     -> CASH/PAYMENTS
                     -> COMPLETED -> DELIVERED
```

Warehouse = physical materials. Cash = money. Material Request bridges repair/warehouse/costing.

## Phase A software blocks

GREEN:

```text
97  Motor winding versions
98  Repair AS_RECEIVED persistence
99  Runtime/read winding + snapshot API
100 Repair intake pending transaction foundation
102 Transactional repair creation + crash recovery
103 Material Request identity + movement schema foundation
104 CRM backup/export + integrity
105 MaterialLedger catalog serialization fix
106 Material Request ↔ MaterialLedger unit adapter + active item lookup
107 Material Request append-only lifecycle + backup/integrity
108 Material Request warehouse pending persistence foundation
```

Checkpoint 108 verification:

```text
implementation head dc73b39e6f7d202d75dae801f5e3413218ca3c0e
ESP32 Build run 32861148982 / SUCCESS
CMP run 32861149158 / SUCCESS
CMP permanent regression run 32861266055 / SUCCESS
```

## Material / warehouse catalog foundation

Existing `/data/materials/materials.ndjson` and `MaterialLedger` are authoritative generic warehouse item catalog. No duplicate catalog.

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

`MaterialLedger::loadActiveMaterialState()` provides ACTIVE item unit/stock/price/currency.

## Material Request current implementation

Durable journals:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
/data/workshop/material-request-status.ndjson
```

Lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Warehouse pending recovery marker foundation:

```text
/data/workshop/material-request-warehouse.pending.json
/data/workshop/material-request-warehouse.pending.tmp
```

It persists the exact operator-confirmed transaction intent/cost/provenance and rejects a second in-flight warehouse transaction.

The full crash-safe warehouse coordinator is NOT complete yet. Stable backup still needs these two paths added to its recovery-marker guard. A one-shot workflow intended to apply that guarded large-file patch was invalid before any job ran and was removed; production backup code was not changed by it.

Movements support:

```text
ISSUE | RETURN | CORRECTION
MANUAL_MATERIAL | RUN_WIRE
KG | L | PCS | M | M2
```

`RUN_WIRE` remains ISSUE/KG-only in the new pending contract and requires exact `source_session_id + source_run_id`, CU/AL and diameter.

## Current NEXT

1. Add warehouse pending/temp paths to stable-backup recovery markers safely.
2. Implement `MaterialRequestWarehouseCoordinator` with idempotent crash recovery.
3. Use movement-first + ledger-second ordering and transaction-ref evidence so stock cannot be silently changed without request evidence.
4. Explicit operator ISSUE/RETURN/CORRECTION only; lifecycle changes remain separate explicit transitions.
5. Then expose bounded request/status/movement/warehouse Web APIs.

## Wire migration

Future contract:

```text
RUN_COMPLETED -> never auto-deducts
operator confirms warehouse ISSUE
material_request_id
exact source_session_id + source_run_id for wire
CU/AL + actual consumed weight
```

Current exact `spool_id` backend/finalization checks remain until coherent migration across job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests.

## Safety invariants

Never weaken:

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
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Hardware acceptance

Not complete. Full two-board E2E remains mandatory after CRM/material/writeoff contracts stabilize.

## Documentation rule

Synchronize 95/101/06/01/90 and update 00 when read order changes. Major persistence/API blocks receive numbered checkpoints with exact commit and CI evidence.
