# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth / stable baseline

Working source-of-truth only `cmp-protocol-v1`. `main` не использовать как source.

```text
stable pre-CRM: 449570d47649d5f6336a31ee3eed491256e0fb1a
main -> same commit
stable-2026-08-25-pre-crm-redesign -> same commit
```

## Current phase

Workshop Web/CRM redesign is GREEN through dedicated Cash Web. Active work is now the coordinated wire-accounting migration.

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
                     -> CASH/PAYMENTS/BALANCE
                     -> COMPLETED -> DELIVERED
```

Warehouse = physical materials. Cash = money. Material Request bridges repair/warehouse/costing.

## Software GREEN checkpoints

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
108 Material Request warehouse pending persistence + stable-backup guard
109 Crash-safe Material Request warehouse coordinator
110 Material Request production runtime/Web API
111 Immutable repair delivery store/API + backup/integrity
112 Append-only cash/payment journal + repair/client balance API + backup/integrity
113 Motor Web catalog-only + separate creation + versioned working card
114 Immutable AS_RECEIVED comparison + role-aware linked WORKING/STARTING job flow
115 Client Web catalog-only + dedicated create + read-only CRM card
116 Dedicated append-only Cash Web UI + BigInt exact money + navigation
```

Latest evidence:

```text
CMP Protocol Tests 32938179528 / SUCCESS
ESP32 Build         32936718747 / SUCCESS
```

## Web/CRM status — GREEN

Motor Web, Client Web and Cash Web are all separated by domain responsibility. Cash uses checkpoint 112 append-only APIs and never owns pricing, warehouse or machine state. Client↔motor relationship remains historical through repair identity.

## Material / warehouse model

`/data/materials/materials.ndjson` and `MaterialLedger` remain the authoritative generic warehouse catalog.

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

Material Request lifecycle remains `DRAFT -> ISSUED -> PRICED -> CLOSED`; warehouse ISSUE/RETURN/CORRECTION are explicit operator actions. `RUN_WIRE` remains ISSUE/KG-only with exact `source_session_id + source_run_id`, CU/AL and diameter. `RUN_COMPLETED` remains non-mutating.

## Delivery / cash — GREEN

Delivery is immutable and separate from repair CLOSED and from cash balance. Debt does not block delivery.

Cash journal is append-only:

```text
PAYMENT    -> ADD only
CORRECTION -> ADD | SUBTRACT
```

Dedicated `/desktop/cash.html` performs only explicit payment/correction appends and exact balance/history reads. Minor units are rendered with `BigInt`; no destructive cash edit/delete exists.

## Current NEXT — coordinated wire migration

First map all current exact-spool owners and persistence boundaries. Then design one coherent compatibility transition toward Material Request-owned run-wire ISSUE.

Future contract:

```text
RUN_COMPLETED -> never auto-deducts
operator confirms warehouse ISSUE
material_request_id
exact source_session_id + source_run_id
CU/AL + actual consumed weight
manual confirmation
```

Current exact `spool_id` backend/finalization/writeoff checks remain authoritative until job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests are migrated together.

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
- cash operations never trigger machine or warehouse mutation;
- run-linked wire movement preserves exact run provenance;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Hardware acceptance

Not complete. Full two-board E2E remains mandatory after CRM/material/writeoff contracts stabilize.

## Documentation rule

Synchronize 95/101/06/01/90 and update 00 when read order changes. Major persistence/API/UI blocks receive numbered checkpoints with exact commit and CI evidence.
