# Активная работа и следующие шаги

Дата обновления: **2026-08-28**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Текущая рабочая ветка: **`arduino-ru-lcd-experiment`**

## Production baseline и experiment policy

Production остаётся без новых переносов:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

Все дальнейшие изменения выполняются только в `arduino-ru-lcd-experiment`. Не переносить их обратно в production без отдельного прямого запроса.

Production CMP Protocol Tests #3772 / run `33141922657` на SHA `28c7917a906bc9b15736369e8986d0e0c354ab8c` завершился FAILURE из-за четырёх stale contract assertions. Runtime rollback не выполнялся. Контракты были выровнены в experiment без ослабления Hall/SSR safety, и experiment CI был добавлен в push coverage.

Подтверждение восстановления:

```text
CMP Protocol Tests #3773
run 33147497236
SHA ea56a53e67500cbee019321542446107e0bc316d
completed / success
```

## Active software block — repair materials and write-off

Authoritative plan:

`docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md`

### Authoritative data flow

Новый material ledger не создаётся.

General repair materials:

```text
/data/materials/materials.ndjson
  -> bounded/searchable GET /api/materials
  -> exact repair + selected material + actual quantity
  -> explicit/manual POST /api/materials/usage
  -> generic operation idempotency lookup
  -> authoritative MaterialLedger state reread / preview checks
  -> MaterialLedger::confirmUsage()
       pending usage WAL
       mutation-time rewriteQuantity() authoritative reread / TOCTOU
  -> /data/materials/usage.ndjson persisted price snapshot
  -> RepairCosting.materialCostMinor
  -> bounded history / integrity / backup
```

RUN_WIRE remains a separate safety path:

```text
exact spool + bridge + immutable selection + completed run evidence
  -> explicit RUN_WIRE Material Request
  -> RunWireIssueCoordinator pending
  -> movement evidence
  -> MaterialLedger usage evidence
  -> managed warehouse physical phases
  -> CONFIRMED movement
  -> RepairCosting.wireCostMinor
  -> separate cross-ledger integrity / backup
```

Exact `spool_id`, `source_session_id`, `source_run_id`, diameter and CU/AL identity remain mandatory. `RUN_COMPLETED` remains evidence only and never deducts wire automatically.

Material Request remains a planning/lifecycle/evidence layer, not a second stock ledger. RepairCosting remains authoritative for aggregation from warehouse movements, material usage and append-only pricing revisions.

## Repair materials checkpoint 1 — repair-card entry — GREEN

```text
c9c74313871e45605520e1b0885cf90ed5978557  desktop repair -> Материалы ремонта
a3aa5c82fcb883eba7156e0f3d249b5ef07ae9b5  mobile repair -> Материалы ремонта
b94325af380788d413aef11b21ff76a48220fdba  repair-material UI contract
f9427b53e18aef047d944169bc9e0639bee78831  mandatory CMP integration
```

Verification:

```text
CMP Protocol Tests #3778
run 33147929516
SHA f9427b53e18aef047d944169bc9e0639bee78831
completed / success
```

## Repair materials checkpoint 2 — bounded history + costing preview — GREEN

Desktop/mobile consume existing authoritative APIs only:

- `GET /api/repairs/costing?repair_id=...` -> current `material_cost_minor`;
- bounded `GET /api/materials/usage?repair_id=...&limit=20&cursor=...`;
- persisted `usage_id`, `material_id`, actual quantity, price snapshot, line cost, timestamp/comment;
- post-write authoritative reread of catalogue, history and costing.

```text
b9057089560f1f862fc36b5ba2d16477edcd0c94  desktop history + costing
0117ae33cb4c28ab801f6263ba7fc2fe1c6dcac3  mobile parity
711e992c9d839d546ee185d246019faf310a88f9  contracts
```

Verification:

```text
CMP Protocol Tests #3782
run 33148352333
SHA 711e992c9d839d546ee185d246019faf310a88f9
completed / success
```

## Repair materials checkpoint 3 — bounded server-side search — GREEN

Search stays streaming over `materials.ndjson`, max query 48 characters, ACTIVE records only, bounded result page/cursor. No DB/index, no whole-file buffering, no unbounded result vector.

```text
4f70357804499ca3d5f617a418220cefc544b1fa  bounded search API contract
649af913fd164241e506cfed61f2dcb75772c3ef  streaming search
2c39e373b8ed7acf28fd93f30e3ed86b021be54f  HTTP search dispatch
a692e51f0ab2dde98181acc43c632deea6a535f6  compile-safety correction
7a0e76f166b7b7dca13bde804684474e5776e5d6  desktop search
73447a822398a36d8516a82fbbe80ff32035344f  mobile parity
7aa81b7119c95d0dc81331cd7c6755fc3e79e44e  contracts
```

Verification:

```text
CMP Protocol Tests #3790
run 33148837612
SHA 7aa81b7119c95d0dc81331cd7c6755fc3e79e44e
completed / success
```

## Repair materials checkpoint 4 — generic duplicate-submit/idempotency — GREEN

Ordinary explicit material usage now has a bounded generic `operation_id`. This is deliberately separate from RUN_WIRE exact-run duplicate protection.

Rules:

- same `operation_id` + same repair/material/quantity -> replay persisted usage snapshot, no second mutation;
- same `operation_id` + different payload -> `operation_id_conflict`, fail closed;
- unknown network retry preserves the same operation id;
- editing selected material/quantity/comment resets operation id for a genuinely new operator action;
- replay scan is streaming/validated; historical rows remain compatible;
- generic idempotency is not imported into `RunWireIssueCoordinator`;
- RUN_WIRE continues to use exact source-session/source-run + spool safety.

Runtime/UI/contract block includes commits through:

```text
e1c00465...  generic operation-id foundation
1f516c45...  persisted replay lookup/tagging
836d050c...  generic material Web idempotency
92ed2e2b...  desktop retry-safe operation id
32283a00...  mobile parity
ba8bc4d0...  initial contract coverage
c314464224a21227f7564bac4ecf5dff85f6f64d  ESP32 experiment trigger/runtime checkpoint
436eddd9ef34f1bcd8165dec0f84f93dd72bbe98  latest contract lineage includes semantic fixes
```

Verified runtime build:

```text
ESP32 Build #1652
run 33149412785
SHA c314464224a21227f7564bac4ecf5dff85f6f64d
completed / success
```

Verified CMP after fixing a stale variable-name assertion only (runtime unchanged):

```text
CMP Protocol Tests #3802
run 33149994140
SHA d9a6eab1f083332214cfe94fa0738d0f35161e24
completed / success
```

## Repair materials checkpoint 5 — stale preview / insufficient stock / TOCTOU UX — GREEN

Backend generic usage preflight now requires client preview snapshots:

```text
expected_stock_quantity_milli
expected_price_per_unit_minor
```

They are guards only. Client preview price is never used as cost authority.

Ordering remains:

```text
operation-id replay lookup
  -> repair lifecycle
  -> authoritative loadActiveMaterialState()
  -> insufficient-stock check
  -> expected/current stock+price stale-preview comparison
  -> MaterialLedger::confirmUsage()
       final authoritative mutation-time reread / TOCTOU + WAL
```

Important semantics:

- idempotency replay happens before stale-preview comparison, so a network retry can safely return a durable prior operation even though current stock changed after that operation;
- `insufficient_stock` returns authoritative available stock and performs no write;
- `stale_material_preview` performs no write and requires the operator to review refreshed stock/price and confirm again manually;
- `usage_not_committed` remains the final mutation-time fail-closed condition; UI refreshes and requires manual retry;
- stale/insufficient branches reset the pending operation id only after the server has explicitly confirmed that no write occurred;
- unknown network failure preserves the same operation id for safe replay;
- no automatic re-submit after refresh;
- desktop/mobile behavior is aligned;
- RUN_WIRE path is untouched.

Implementation:

```text
43b640a32c5ad72d6de9f2b8f1835f175fe0a34f  backend stale-preview/stock preflight
a4220a72986525508e9917744acc6db453589426  desktop manual reconfirmation
ddecc6108d1219fed4a80f7c98d942a45d9d2e20  mobile parity
5aafd439909206cdc1e7f440a202327b605d73f8  initial checkpoint-5 contract
436eddd9ef34f1bcd8165dec0f84f93dd72bbe98  semantic escaped-error contract fix
```

The first CMP run #3806 / `33151003904` failed only because the new test searched for an unescaped C++ JSON literal. Configure/build/ctest and the underlying material safety audit passed. The assertion was changed to semantic error tokens; runtime was not changed.

Final verification:

```text
CMP Protocol Tests #3807
run 33151095290
SHA 436eddd9ef34f1bcd8165dec0f84f93dd72bbe98
completed / success

ESP32 Build #1655
run 33150952500
SHA ddecc6108d1219fed4a80f7c98d942a45d9d2e20
completed / success
```

## Repair materials checkpoint 6 — ambiguous outcome / same-operation retry UX — GREEN

`MaterialLedger::confirmUsage()` can fail after `rewriteQuantity()` has already changed authoritative stock but before usage append/pending cleanup has fully completed. Therefore `usage_not_committed` is an ambiguous mutation outcome and must never silently become a new operator operation.

Desktop/mobile now enforce:

- `usage_not_committed` preserves the existing `pendingOperationId`;
- operator is told not to change material, quantity or comment before retry;
- retry uses the same operation id so durable idempotency/recovery evidence can resolve the result without a second writeoff;
- catalogue, durable usage history and costing are refreshed for operator visibility;
- unknown transport/error catch also preserves the same operation id and explicitly warns not to change the operation row;
- stale-preview and insufficient-stock branches still reset operation id because those are proven pre-mutation no-write results;
- no automatic retry/recovery was added;
- RUN_WIRE coordinator and exact-run duplicate protection remain untouched.

Implementation:

```text
2378dd6a17ef2db1f4601b0d8acd56e3390fd1e0  desktop ambiguous retry preserves operation id
bada572285bb098e373da63e0ec1681a751ef608  mobile parity
53238fcb898fc3e4fc19a2dece3cfaf41c65c5ca  contract: ambiguous branch cannot reset operation id
```

The intermediate CMP #3810 on the mobile runtime commit failed before the new contract file was aligned. Final contract checkpoint is GREEN.

Final verification:

```text
CMP Protocol Tests #3811
run 33153190389
SHA 53238fcb898fc3e4fc19a2dece3cfaf41c65c5ca
completed / success

ESP32 Build #1657
run 33153146103
SHA bada572285bb098e373da63e0ec1681a751ef608
completed / success
```

## Repair materials checkpoint 7 — unified visible RUN_WIRE entry, separate exact-safe mutation path — GREEN

The repair-material page now exposes RUN_WIRE in the same operator surface, but does not duplicate or merge the RUN_WIRE state machine into generic material usage.

Desktop/mobile `materials.html` now show a dedicated `Провод RUN_WIRE` block and pass only the exact `repair_id` to the existing dedicated `writeoff.html` workflow.

The generic material page deliberately does **not**:

- POST to `/api/material-requests/warehouse`;
- construct `source_session_id`, `source_run_id` or `spool_id`;
- route RUN_WIRE through `/api/materials/usage`;
- infer or weaken provenance requirements.

The existing shared writeoff workflow remains authoritative and still obtains:

```text
RUN_COMPLETED evidence          <- /api/winding-history
already-covered exact runs      <- /api/warehouse/write-offs
immutable spool selection       <- /api/jobs/spool-selection
active spool identity           <- /api/warehouse/spools
spool -> MaterialLedger bridge  <- /api/warehouse/spool-material-bridges
repair Material Request/status  <- /api/material-requests + /status
```

Only after those checks does the operator manually submit `confirmed=true`, exact session/run/spool/diameter/material identity to `/api/material-requests/warehouse`, which remains routed to `RunWireIssueCoordinator::execute()`.

Implementation:

```text
aab2442e67ceed89e1beee382206f6054b88bedb  desktop unified RUN_WIRE entry
e5b632b5e7f713b0cce71ef4e643a6e1251cc229  mobile parity
bd61c9b4b08fa3490c99406e5453e96b5bc28560  contract: generic material page cannot duplicate exact RUN_WIRE mutation/provenance
```

Final verification:

```text
CMP Protocol Tests #3815
run 33153831232
SHA bd61c9b4b08fa3490c99406e5453e96b5bc28560
completed / success

ESP32 Build #1659
run 33153753512
SHA e5b632b5e7f713b0cce71ef4e643a6e1251cc229
completed / success
```

## Current execution order

1. Continue only in `arduino-ru-lcd-experiment`.
2. Audit and implement append-only correction/history UX for ordinary repair materials using existing authoritative adjustment/event semantics where possible.
3. Confirmed usage/writeoff rows must never be edited or deleted; corrections must be new durable records with explicit provenance to the corrected operation where the existing schema supports it.
4. Corrections must remain reflected in authoritative stock/costing/integrity/backup; do not create a UI-only compensation layer.
5. Keep generic material corrections separate from exact RUN_WIRE warehouse movement correction semantics unless the existing backend already provides a safe shared abstraction.
6. Preserve no-auto-writeoff, no-auto-retry and no automatic physical START semantics.
7. Verify actual CMP/ESP32 CI and update PROJECT_HANDOFF after each completed checkpoint.
8. Do not copy experiment commits into `cmp-protocol-v1` until separately requested.

## Safety invariants

- no automatic physical START or repeat START;
- no auto-resume after reboot;
- Arduino is the only SSR owner;
- ESP32/Web never controls SSR directly;
- `RUN_COMPLETED` is evidence only;
- RUN_WIRE writeoff is explicit/manual;
- exact `spool_id` + `source_session_id` + `source_run_id` remain mandatory;
- restore/recovery remain fail-closed/operator-controlled;
- MaterialLedger `confirmUsage()` retains authoritative reread and final mutation-time TOCTOU protection;
- generic material idempotency never replaces RUN_WIRE exact-run protection;
- different integrity ledgers and phases remain separate;
- no unbounded whole-file buffering of growing NDJSON;
- no automatic production truncation/rotation;
- no premature DB/index migration.

## Hardware acceptance

Full Arduino + ESP32 two-board E2E remains required before final project completion. For this material software block, hardware E2E remains deferred until software checkpoints are complete unless a concrete hardware-dependent issue requires earlier verification.
