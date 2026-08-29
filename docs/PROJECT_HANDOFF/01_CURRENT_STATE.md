# Текущее состояние CoilMaster

Дата обновления: **2026-08-29**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Текущая рабочая ветка: **`arduino-ru-lcd-experiment`**

## Source of truth / branch policy

Production остаётся на `cmp-protocol-v1`; `main` для исходников не использовать.

Текущую дальнейшую разработку и experiment-side optimization выполнять только в `arduino-ru-lcd-experiment`. Не переносить experiment обратно в production без отдельного прямого запроса пользователя.

Production не изменён:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

## Current phase

Production behavior подтверждён GREEN through checkpoint **165**. Production checkpoint **166** закрыт как residual audit **NO-CHANGE** и production software optimization остаётся frozen до hardware E2E либо конкретного дефекта.

После синхронизации production дальнейшие изменения продолжаются отдельно в `arduino-ru-lcd-experiment`. Experiment-side repeated-scan/performance work подтверждён through checkpoint **158**. Experiment checkpoint **159** закрывает отдельный функциональный дефект autonomous winding → canonical motor winding history. Full two-board Arduino + ESP32 hardware E2E остаётся отдельным финальным acceptance gate.

## Production optimization checkpoints

```text
148 managed RUN_WIRE spool mutation removes redundant pre-scan
149 spool/material bridge append -> one validated bridge-log pass
150 MaterialLedger confirmUsage two-pass retained as safety boundary (NO-CHANGE)
151 append-only audit -> no safe same-ledger duplicate full scan (NO-CHANGE)
152 autonomous save -> one bounded-tail latest-event read
153 dead-helper linker/flash audit -> linker GC already removes unused helpers (NO-CHANGE)
154 autonomous task query parsed once per page
155 motor similarity candidate parsed once; each stored winding program parsed once
156 motor similarity Web handler reuses one coil_program request String
157 material history optional query values fetched once
158 material adjustment optional quantity/price values fetched once
159 standard conductor recommendations reuse one warehouse availability lookup per component
160 calculator warehouse diameter lookup -> binary search over sorted fixed catalogue
161 warehouse catalogue stays sorted during scan -> binary lookup + bounded insertion, no final O(N^2) sort
162 recommendation search reuses precomputed single-wire areas across strand combinations
163 recommendation top-3 caches rankingScore; existing scores are not recalculated per candidate
164 calculator Web request reuses one required target area for warehouse + standard searches + JSON
165 required target area derives from already-cached sourceArea; removes second source-component area pass
166 final residual audit -> NO-CHANGE
```

Production commits:

```text
1e831cde072f6c6152d10b7e71cb6a1e0f2a7b0e  checkpoint 152
a2f98cb377873d88d3fd103b6dfdfbabaf28ea65  checkpoint 154
415394d162de0f1c83e433cbbea3db94833b3162  checkpoint 155
78b41d38abdf89b9e72a02eea37edcd346c9610f  checkpoint 156
491fcf965fe573f91eb29bc99f6513017f3f5b1a  checkpoint 157
0596ae9ff473503bd1d21aeda0c6c4da0f2ba0da  checkpoint 158
93d858c83b3f63932d6c2809df585a017a74a6b6  checkpoint 159
1efb3c35947c77fb79b5cc7a24f0c07c5dcab67c  checkpoint 160
317273ac74e6e67208e9a94330b615bb3ba1ba08  checkpoint 161
18d611e6ee8bb0355deda5f99874b0b9923d576f  checkpoint 162
ac5411cc7ad2f279eef655fc3b0e3be3f139b4d0  checkpoint 163
e2d84e5ab37ec89724c8a1f71d5f29ddd62c5cea  checkpoint 164
db642c50a79d80179a765c5c4ff8ebb5006fd27f  checkpoint 165 final production code
```

## Latest verified production CI evidence

```text
CMP Tests #3766    33047940015 / SUCCESS
ESP32 Build #1650  33047940040 / SUCCESS
CMP Tests #3767    33048020592 / SUCCESS
```

## Experiment checkpoints 152–159

Experiment branch includes the separate RUN_WIRE Material Request batching, unified autonomous/Web archive, exact immutable-spool lookup, repeated-scan reductions through checkpoint 158, and checkpoint 159 canonical autonomous-winding projection documented in `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

Checkpoint **157 — GREEN** removed repeated full `repairs.ndjson` validation from client balance while preserving generic/mutation repair validation. Runtime `bf529900a8211f0b9a920ec237942bae2f7093c5` is verified by CMP #3939, ESP32 #1743 and Arduino RU LCD #167; contract `19bf03003b5e1f3aea6692609e634a79247fb397` is verified by CMP #3941.

Checkpoint **158 — GREEN**:

- `RepairCostingWeb::handlePricingHistory()`, `handleGet()` and the read/preflight stage of `handleSavePricing()` each already perform an authoritative exact `RepairCosting::repairExists(repairId, found)` check;
- after that successful proof they call `RepairCosting::loadKnownRepair()` instead of immediately invoking generic `load()` and scanning `repairs.ndjson` a second time;
- HTTP 404/read-failure semantics of the initial exact repair lookup are unchanged;
- `RepairCosting::load()` still retains its own exact repair validation for all callers without prior proof;
- `RepairCosting::savePricing()` remains unchanged and still performs its own repair-lifecycle gate plus generic `load()` before the append, preserving mutation-time authoritative validation;
- pricing-history still keeps its separate pricing-history scan because history/current comparison is an explicit integrity contract;
- all Warehouse movement, material usage/correction, pricing currency/overflow, RUN_WIRE pending and append-only validations remain unchanged;
- no cache/index/DB or unbounded state added.

Runtime:

```text
e460a32a0021b49a6d5262c316a1d9f83f5554d2
CMP Protocol Tests #3944  run 33195340340 / SUCCESS
ESP32 Build #1744         run 33195340296 / SUCCESS
Arduino RU LCD #168       run 33195340333 / SUCCESS
```

Regression contract:

```text
3f5d7f65782196491cf81934d0b3aa0276914a02
CMP Protocol Tests #3945  run 33195389757 / SUCCESS
```

Adjacent costing audit: moving `MaterialUsageCorrectionIntegrityAudit::check()` out of each known-repair load was classified **NO-CHANGE**. That audit is global provenance validation over usage/adjustment history; skipping repeated calls safely would require a new prevalidated bypass/context or a substantially larger batch-costing API, increasing integrity risk and code/flash complexity.

The CashPayment correction preflight also remains **NO-CHANGE**: combining `eventBelongsToRepair()` and `totalsForRepair()` would require parallel validation logic and risk changing fail-closed/error semantics. Mutation-time `append()` authoritative scan remains mandatory.

### Checkpoint 159 — autonomous winding canonical motor projection — GREEN

The defect where an autonomous/completed winding could be linked in the Arduino archive but remain absent from the normal motor card is closed on `arduino-ru-lcd-experiment`.

Semantics now fixed:

- assignment projects the winding into append-only `MotorWindingVersionStore` used by the normal motor card;
- canonical roles are only `WORKING` and `STARTING`; `AUXILIARY` is rejected and removed from the runtime operator selector;
- exact projection identity is `session_id + run_id + role`, so identical retry does not create another canonical version;
- historical assignment-only records are backfilled on retry because canonical projection is checked/performed before the assignment append path completes;
- canonical-first partial failure is retry-safe: an already-created projection is detected by provenance and the missing assignment ledger append can complete without duplicating motor history;
- adding/replacing one role preserves the complete other role from the latest motor winding version;
- `STARTING` without an existing `WORKING` winding fails closed;
- an occupied target role returns conflict and is never silently overwritten;
- replacement requires explicit `replace_existing=true` and appends a new canonical version; UI defaults to false and does not automatically retry a 409 conflict;
- local autonomous projection derives the canonical program from completed autonomous evidence; completed ESP32/Web jobs use persisted immutable job state/snapshot data;
- no physical RUN evidence is copied or rewritten;
- no cache/index/DB, whole-file buffering, automatic truncation or history mutation was introduced.

Key commits in the block:

```text
348b75c  canonical autonomous provenance/API
0066262d projection/retry/role-preservation implementation
9e7b1390d7394ccbaddf0942b00085d859f8a0be C++/Web syntax-fixed runtime checkpoint
e1450325a88b85464029f3ec7c312ca88f435cc9 explicit operator replacement safety UI
8955f506f793b8a732a89726f38708ba2520945d UI regression contract
```

Verified CI evidence:

```text
9e7b1390d7394ccbaddf0942b00085d859f8a0be
CMP Protocol Tests #3957 / SUCCESS
ESP32 Build #1751        / SUCCESS

8955f506f793b8a732a89726f38708ba2520945d
CMP Protocol Tests #3959 run 33254003888 / SUCCESS
```

The post-`9e7b1390...` commits modify Web JS/tests only; no C++ firmware source changed after the ESP32 #1751 compile checkpoint.

## Safety / integrity boundaries that remain intentionally unchanged

- No automatic physical START, repeat START or resume.
- Arduino owns SSR; ESP32/Web never drives SSR directly.
- `RUN_COMPLETED` is evidence only and never automatically deducts wire.
- Wire write-off remains explicit/manual and tied to exact `spool_id`, `source_session_id`, `source_run_id`.
- MaterialLedger `confirmUsage()` keeps separate pre-WAL and mutation-time authoritative reads.
- Different ledgers / different mutation phases keep separate validation passes when they are integrity boundaries.
- No tail-only substitute for authoritative historical integrity scans.
- No unbounded vectors or whole-file buffering for growing NDJSON.
- No automatic production-data rotation/deletion/truncation and no premature DB/index migration.

## Current NEXT

1. Continue only in `arduino-ru-lcd-experiment`; production remains unchanged.
2. Treat checkpoint 159 autonomous → canonical motor-card projection as closed unless a new concrete regression is observed.
3. Resume repo-reviewable repeated-scan/performance audit only where the same request repeats an authoritative growing-journal pass and the proof can be preserved.
4. Inspect remaining exact-proof → generic-load/read patterns outside RepairCostingWeb before considering any larger batch costing refactor.
5. Keep global correction-integrity reuse and CashPayment correction fusion as NO-CHANGE unless a simpler proof-preserving primitive appears.
6. Full two-board Arduino + ESP32 hardware E2E remains the final separate acceptance gate before final release completion.

## Hardware acceptance

Full two-board Arduino + ESP32 E2E remains required before final project completion, but current repo-reviewable experiment work may continue without intermediate hardware tests.
