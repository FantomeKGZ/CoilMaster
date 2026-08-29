# Активная работа и следующие шаги

Дата обновления: **2026-08-29**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Текущая рабочая ветка: **`arduino-ru-lcd-experiment`**

## Branch policy

Production не изменён:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

Все новые изменения выполнять только в `arduino-ru-lcd-experiment`. Не переносить experiment обратно в production без отдельного прямого запроса пользователя. `main` не использовать как source.

## Active state

Experiment-side repo-reviewable software work закрыт through checkpoint **165**.

Ключевые detailed records:

```text
07_REPAIR_MATERIAL_WRITEOFF_PLAN.md
10_CHECKPOINT_161_WAREHOUSE_PROVENANCE_SUFFIX_SCAN.md
11_CHECKPOINT_162_REPAIR_FINALIZATION_KNOWN_REPAIR.md
12_CHECKPOINT_163_165_REPEATED_SCAN_CLOSEOUT.md
```

`01_CURRENT_STATE.md` остаётся общей полной сводкой состояния и CI evidence.

## Закрытые software-блоки

### Repair materials / accounting / integrity — GREEN

Authoritative plan: `07_REPAIR_MATERIAL_WRITEOFF_PLAN.md`.

Сохраняются:

- generic repair-material usage только explicit/manual;
- bounded catalogue/search/history;
- operation-id idempotency;
- authoritative stock/price preview + final mutation-time TOCTOU/WAL;
- persisted price snapshot;
- append-only corrections with exact source usage provenance;
- cumulative over-correction guard;
- net RepairCosting;
- correction integrity + backup coverage;
- RUN_WIRE как отдельный exact-safe Warehouse path;
- desktop/mobile parity.

### Checkpoints 152–158 — GREEN

```text
152 RUN_WIRE Material Request status batching
153 unified autonomous/Web completed-job archive lifecycle
154 RUN_WIRE exact immutable-spool lookup
155 Material Request create repair scan reuse
156 Material Request Warehouse known-request status reuse
157 client balance repair-journal validation reuse
158 RepairCostingWeb exact repair proof reuse
```

Safety behavior этих блоков не менять. Generic callers остаются self-validating; mutation-time authoritative rereads не удалять.

### Checkpoint 159 — autonomous winding → canonical motor history — GREEN

- autonomous/completed assignment projects into append-only `MotorWindingVersionStore`;
- canonical roles only `WORKING` and `STARTING`;
- exact retry identity `session_id + run_id + role`;
- historical assignment-only rows backfill on retry;
- canonical-first partial failure is retry-safe;
- untargeted role is fully preserved;
- `STARTING` requires existing `WORKING`;
- occupied role never silently overwrites;
- replacement requires explicit `replace_existing=true` and appends a new version;
- UI defaults replacement to false and does not auto-retry 409;
- no physical RUN evidence is fabricated/copied.

Verified block evidence:

```text
9e7b1390d7394ccbaddf0942b00085d859f8a0be
CMP Protocol Tests #3957 / SUCCESS
ESP32 Build #1751        / SUCCESS

8955f506f793b8a732a89726f38708ba2520945d
CMP Protocol Tests #3959 run 33254003888 / SUCCESS
```

### Checkpoints 160–161 — Warehouse growing-journal optimizations — GREEN

Checkpoint 160 removed a confirmed exact lookup duplicate scan.

Checkpoint 161 changed Warehouse CONFIRMED provenance uniqueness validation so each fixed batch compares against the later suffix only; already-proven prefix is not reread. Fixed RAM bounds and full transaction/schema/order validation remain.

```text
dc9415c531d8c9685bc6202941df042ec299af0c
CMP Protocol Tests #3965  run 33257271690 / SUCCESS
ESP32 Build #1754         run 33257271722 / SUCCESS
Arduino RU LCD #178       run 33257271706 / SUCCESS

875c6a069b3680569dc35576d82861d737444144
CMP Protocol Tests #3966  run 33257294547 / SUCCESS
```

Detailed record: `10_CHECKPOINT_161_WAREHOUSE_PROVENANCE_SUFFIX_SCAN.md`.

### Checkpoint 162 — repair finalization known-repair proof reuse — GREEN

Both RepairRegistryWeb finalization flows already prove exact repair/open state and now pass that proof into `RepairFinalizationGuard::checkKnownRepair()` / `RepairCosting::loadKnownRepair()` instead of immediately rescanning `repairs.ndjson`.

Generic finalization remains self-validating. `handleCloseRepair()` still executes mutation-time `m_registry.closeRepair(...)`, preserving the TOCTOU boundary.

```text
ESP32 Build #1757         run 33257746469 / SUCCESS
Arduino RU LCD #181       run 33257746498 / SUCCESS
CMP Protocol Tests #3971  run 33257805004 / SUCCESS
```

Detailed record: `11_CHECKPOINT_162_REPAIR_FINALIZATION_KNOWN_REPAIR.md`.

### Checkpoint 163 — Repair Delivery single-pass preflight — GREEN

Repair Delivery append obtains both existing-repair linkage/conflict state and next `delivery_id` from one authoritative validated delivery-journal pass via:

```cpp
prepareAppend(repairId, deliveryId, alreadyExists)
```

Exact repair identity is reused for the status-only open-state lookup. Mutation/recovery/TOCTOU semantics remain authoritative.

Final verified evidence:

```text
ESP32 Build #1774         run 33265057626 / SUCCESS
Arduino RU LCD #198       run 33265057605 / SUCCESS
CMP Protocol Tests #4009  run 33265221419 / SUCCESS
CMP Protocol Tests #4010  run 33265276200 / SUCCESS
checkpoint docs HEAD      eb22812e4cec4954a1ca2aedf730128cbb4b6742
```

The earlier `#4005–#4008`, `#1773`, `#197` were intermediate failures and are not GREEN evidence.

### Checkpoint 164 — spool/material bridge suffix uniqueness audit — GREEN

`SpoolMaterialBridgeStore::validateAll()` now:

- streams the outer bridge journal once in fixed `BridgeAuditBatchSize = 24` batches;
- validates strict `bridge_id` growth;
- rejects duplicate `spool_id` inside the current batch in bounded RAM;
- records `outer.position()` after the batch;
- compares that batch only with the still-unseen suffix;
- no longer rereads the already-proven prefix for every batch.

Commits:

```text
d8862aef7ae3b3c4a6e3e7dbbe49c92d19babb77  implementation
63c70e59fee99f77c20135606c8d9911f8bfbd4e  scoped contract
8f37cb5268ee461e9b41a2307981d3fd45b9a565  stale contract correction
fb7aaa368ae21fe5041395f0df5eef959233920d  final namespace-close fix
```

Intermediate failures are intentional history, not hidden:

```text
CMP #4011   run 33266038272 / FAILURE  stale full-rescan source-text assertion
ESP32 #1776 run 33266038221 / FAILURE  missing final namespace CM brace
```

Final exact-head evidence:

```text
fb7aaa368ae21fe5041395f0df5eef959233920d
CMP Protocol Tests #4014  run 33266181118 / SUCCESS
ESP32 Build #1777         run 33266181104 / SUCCESS
```

### Checkpoint 165 — residual repeated-scan audit — NO-CHANGE

The remaining strongest candidates were reviewed and must stay unchanged:

- `SpoolMaterialBridgeIntegrityAudit`: bridge batches can reference arbitrary rows anywhere in `spools.ndjson` and `materials.ndjson`; suffix-only reference lookup would miss valid prefix references. A safe removal would require a new persistent index/cache or whole-file state, which is intentionally rejected.
- `MaterialUsageCorrectionIntegrityAudit`: every fixed batch must see previous corrections to prove cumulative over-correction limits, operation uniqueness and source provenance.
- CashPayment correction/read preflight vs `append()`/`analyzeAppendState()`: separate read and mutation phases; mutation-time authoritative scan remains mandatory.
- Repair Intake/recovery: rereads around durable pending/append/recovery are intentional TOCTOU/recovery boundaries.

Result: checkpoint 165 is **NO-CHANGE**. Do not continue speculative repeated-scan refactors solely to reduce file opens.

Detailed closeout: `12_CHECKPOINT_163_165_REPEATED_SCAN_CLOSEOUT.md`.

## Current execution order

1. Continue only in `arduino-ru-lcd-experiment`; production stays at `28c7917...`.
2. Treat checkpoints 159–165 as closed unless a concrete regression is observed.
3. Repeated-scan optimization is temporarily exhausted; resume it only for a concrete measured bottleneck or defect.
4. Next repo-reviewable work should target a concrete experiment defect or remaining Hall/RU-LCD acceptance work, not speculative storage refactoring.
5. Keep `MaterialUsageCorrectionIntegrityAudit` batch rereads **NO-CHANGE**.
6. Keep CashPayment mutation-time authoritative reread **NO-CHANGE** unless a proof-preserving mutation primitive is explicitly introduced.
7. Never remove recovery or mutation-time TOCTOU rereads solely for performance.
8. Preserve separate integrity domains; do not fuse unrelated ledgers only to reduce I/O.
9. Keep fixed RAM bounds: no whole-file buffering, unbounded vectors or caches on ESP32.
10. No automatic production rotation/deletion/truncation and no premature DB/index migration.
11. Full Arduino+ESP32 hardware E2E remains the final release acceptance gate.
12. Do not copy experiment commits into `cmp-protocol-v1` until separately requested.

## Safety invariants

- no automatic physical START/repeat START;
- no auto-resume after reboot;
- Arduino is the only SSR owner;
- ESP32/Web never controls SSR directly;
- `RUN_COMPLETED` is evidence only;
- RUN_WIRE writeoff remains explicit/manual;
- exact `spool_id + source_session_id + source_run_id` mandatory;
- restore/recovery remain fail-closed/operator-controlled;
- MaterialLedger keeps authoritative reread and mutation-time TOCTOU protection;
- generic material idempotency never replaces RUN_WIRE exact-run protection;
- confirmed append-only history is never silently edited/deleted;
- no unbounded growing-NDJSON buffering/automatic truncation/early DB migration.
