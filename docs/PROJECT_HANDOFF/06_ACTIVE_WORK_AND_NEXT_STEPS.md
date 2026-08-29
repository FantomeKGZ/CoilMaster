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

## Закрытые текущие software-блоки

### Repair materials — software accounting/integrity accepted

Authoritative plan: `07_REPAIR_MATERIAL_WRITEOFF_PLAN.md`.

Core flow закрыт checkpoint 151:

- generic repair-material usage — explicit/manual;
- bounded catalogue/search/history;
- operation-id idempotency;
- authoritative stock/price preview + final mutation-time TOCTOU/WAL;
- persisted price snapshot;
- append-only generic corrections with exact source usage provenance;
- cumulative over-correction guard;
- net RepairCosting;
- correction integrity/backup coverage;
- RUN_WIRE остаётся отдельным exact-safe warehouse path;
- desktop/mobile parity.

### Client editing + PREPAYMENT — GREEN

Checkpoint 149:

- stable `client_id`;
- append-only client revisions;
- current by-id/list/search;
- client edit desktop/mobile;
- PREPAYMENT в существующем CashPayment journal;
- `ADD`, exact `client_id`, `repair_id=0`, explicit confirmation;
- отдельный `prepayment_minor`;
- никакого автоматического зачёта в ремонт/RepairCosting.

### Material fail-closed operator UX — GREEN

Checkpoint 150:

- ambiguous/idempotency authoritative failures сохраняют тот же operation id;
- durable replay может уже существовать — новый id запрещён;
- proven pre-mutation no-write требует нового ручного подтверждения;
- автоматического retry нет.

### RUN_WIRE Material Request status batching — GREEN

Checkpoint 152 устранил N+1 status scans в RUN_WIRE preflight.

```text
runtime 104f319e1cb8e466a229c0d40876b35aac6ded86
ESP32 Build #1708         33167009269 / SUCCESS
Arduino RU LCD #132       33167009264 / SUCCESS
contract 17c7c43058802f44d5f0b26d0da301190e792ebd
CMP Protocol Tests #3885  33167039155 / SUCCESS
```

Bounded max batch remains 24 ids; request/status journals are each streamed once per bounded page; exact lifecycle and manual RUN_WIRE provenance remain fail-closed.

### Unified autonomous/Web completed-job archive lifecycle — GREEN

Checkpoint 153:

```text
a23d681a58670866f281191e7d1be6c5915d11f9
CMP Protocol Tests #3920 33174170271 / SUCCESS
ESP32 Build #1734        33174170232 / SUCCESS
Arduino RU LCD #158      33174170231 / SUCCESS
```

Long Arduino `session_id/run_id` remain authoritative. UI short numbering is presentation-only. `ESP32_JOB` projection never fabricates physical RUN evidence. Exact linkage is first-write-only and duplicate conflicting provenance remains fail-closed. `web-assignments.ndjson` is append-only, integrity-audited and backed up.

### RUN_WIRE exact immutable-spool lookup — GREEN

Checkpoint 154:

```text
runtime/UI 42ff9c2f8bbbc1d65a945ff019502299197e9b76
ESP32 Build #1736         33174983830 / SUCCESS
Arduino RU LCD #160       33174983809 / SUCCESS
contract fd3428cf2c9ebdafe7a6dfe20206281701d43f94
CMP Protocol Tests #3926  33175377848 / SUCCESS
```

Exact `spool_id` now resolves through authoritative by-id lookup instead of browser paging of the full active-spool catalogue. 404 remains fail-closed; no alternate spool is inferred. Exact `source_session_id + source_run_id + spool_id` and manual confirmation are unchanged.

### Material Request create repair scan reuse — GREEN

Checkpoint 155:

```text
runtime 28d4704819fc2e09443448a3766d91974431f136
CMP Protocol Tests #3930 33181705062 / SUCCESS
ESP32 Build #1738        33181705074 / SUCCESS
contract 06f814922c5e081059545e3c4389a83f543efba0
CMP Protocol Tests #3931 33181752412 / SUCCESS
```

`POST /api/material-requests` performs authoritative `loadRepairIdentity()` once, then status-only open check. Generic `repairIsOpen()` retains exact existence validation for unrelated callers. Repair status fail-closed semantics are unchanged.

### Material Request Warehouse known-request status reuse — GREEN

Checkpoint 156:

```text
runtime e9756e7c176fac6030c6adea68519695804b47e3
CMP Protocol Tests #3934 33182699055 / SUCCESS
ESP32 Build #1740        33182699044 / SUCCESS
contract 59f20bc192bd09a5012441aafec84fe726bec2cc
CMP Protocol Tests #3935 33183113778 / SUCCESS
```

Fresh `buildPending()` proves exact request/repair identity first and then scans status only. Recovery remains stricter and still revalidates request existence through the general resolver. Movement-first WAL ordering, pending recovery, repair-open gate, authoritative material state and mutation-time ledger checks are unchanged.

Adjacent audits after checkpoint 156:

- Material Request movement/history — NO-CHANGE;
- Material Request status transition — NO-CHANGE;
- MaterialLedger pre-read + mutation-time reread retained — NO-CHANGE;
- Repair lookup/list and Warehouse exact/list paths already bounded/one-pass per request — NO-CHANGE;
- Repair pricing-history second pricing scan retained because history/current comparison is an explicit integrity contract — NO-CHANGE;
- CashPayment correction `eventBelongsToRepair()` + SUBTRACT totals candidate — NO-CHANGE because a fused helper would duplicate validation logic and risk changing existing error/fail-closed semantics.

### Client balance repair-journal validation reuse — GREEN

Checkpoint 157 removes repeated full `repairs.ndjson` validation from one client-balance request without weakening generic costing or mutation paths.

Runtime commits:

```text
e50b09bc61ca3f3a4a053a5dd826f25d36ad6c71  add RepairCosting::loadKnownRepair()
347ca06061129afe223e6fa56b7379315cfca38b  generic load validates repair then delegates
bf529900a8211f0b9a920ec237942bae2f7093c5  validate client repairs once per balance request
```

Runtime verification:

```text
CMP Protocol Tests #3939 33191506843 / SUCCESS
ESP32 Build #1743        33191506805 / SUCCESS
Arduino RU LCD #167      33191506861 / SUCCESS
```

Final regression contract:

```text
19bf03003b5e1f3aea6692609e634a79247fb397
CMP Protocol Tests #3941 33194910481 / SUCCESS
```

Semantics:

- client balance obtains repair ids only from bounded client-filtered RepairRegistry pages;
- after the first returned repair id, one authoritative full `repairExists()` validates `repairs.ndjson` for the request;
- subsequent repair ids use `loadKnownRepair()` and do not repeat the full repair-journal scan;
- generic `RepairCosting::load()` still always performs `repairExists()` before delegating;
- `savePricing()` still uses generic `load()`; mutation semantics are unchanged;
- known-repair path skips only the already-proven repair-existence scan; all warehouse movement, material usage/correction, pricing, currency, overflow and RUN_WIRE pending checks remain identical;
- no persistent cache/index/database or unbounded collection added.

Intermediate contract commit `6a5ecda6eb514f65152d4d6ff1b00a7f995f109d` produced CMP #3940 FAILURE only because two source-text assertions expected different local spelling. Runtime was unchanged; `19bf0300...` corrected only assertions.

### RepairCostingWeb exact repair proof reuse — GREEN

Checkpoint 158 removes three immediate duplicate full `repairs.ndjson` scans after exact repair proof in the costing Web layer.

Runtime:

```text
e460a32a0021b49a6d5262c316a1d9f83f5554d2
CMP Protocol Tests #3944 33195340340 / SUCCESS
ESP32 Build #1744        33195340296 / SUCCESS
Arduino RU LCD #168      33195340333 / SUCCESS
```

Regression contract:

```text
3f5d7f65782196491cf81934d0b3aa0276914a02
CMP Protocol Tests #3945 33195389757 / SUCCESS
```

Semantics:

- `handlePricingHistory()`, `handleGet()` and the read/preflight stage of `handleSavePricing()` retain their existing exact `m_costing.repairExists(repairId, repairFound)` lookup and 404/read-failure semantics;
- after successful exact proof they use `m_costing.loadKnownRepair()` rather than generic `load()` and therefore avoid an immediate second `repairs.ndjson` pass;
- generic `RepairCosting::load()` is unchanged and still owns repair existence validation for callers that do not already have proof;
- `RepairCosting::savePricing()` is unchanged and still performs its own repair-lifecycle gate plus generic `load()` before append, preserving mutation-time authoritative validation;
- pricing-history's second `pricing.ndjson` scan remains intentionally retained for current-vs-history integrity comparisons;
- all movement/material/correction/pricing integrity checks inside costing remain unchanged;
- no cache/index/DB or new unbounded state added.

Adjacent checkpoint-158 audit:

- moving `MaterialUsageCorrectionIntegrityAudit::check()` out of each known-repair costing load is **NO-CHANGE**. The audit is global provenance validation that internally processes corrections in bounded batches and rereads usage/adjustments. Avoiding repeated calls would require a new prevalidated bypass/context or a larger batch-costing API, which adds integrity and code/flash risk.

### Autonomous winding → canonical motor winding history — GREEN

Checkpoint 159 closes the functional defect where Arduino archive assignment was durable in the autonomous assignment ledger but could remain absent from the normal motor card because `MotorWindingVersionStore` was not updated.

Implemented behavior:

- both local autonomous RUN and completed ESP32/Web job assignments project to append-only canonical `MotorWindingVersionStore`;
- exact canonical provenance is `source_autonomous_session_id + source_autonomous_run_id + source_autonomous_role`;
- identical retry is idempotent and does not append a duplicate canonical winding version;
- historical assignment-only entries are backfilled on retry;
- projection executes before assignment-ledger completion, and provenance detection makes canonical-first/assignment-failed retry safe;
- only canonical `WORKING` and `STARTING` roles are accepted; runtime UI removes `AUXILIARY` and backend rejects unsupported roles;
- adding one role preserves the complete existing other role;
- `STARTING` on a motor without existing `WORKING` fails closed;
- occupied target role returns HTTP 409 and is never silently overwritten;
- replacement requires explicit `replace_existing=true`, creates a new append-only canonical version and never mutates old history;
- operator UI defaults `replace_existing=false`, exposes a separate explicit replacement checkbox, and never automatically retries the 409 conflict;
- physical RUN evidence remains immutable/not copied; SSR ownership, START behavior and material writeoff are unchanged;
- no unbounded buffering/cache/index/database/history truncation introduced.

Key commits:

```text
348b75c canonical provenance/API
0066262d projection, retry/backfill, role-preservation logic
9e7b1390d7394ccbaddf0942b00085d859f8a0be syntax-fixed C++ runtime checkpoint
e1450325a88b85464029f3ec7c312ca88f435cc9 explicit operator replacement safety UI
8955f506f793b8a732a89726f38708ba2520945d UI contract
```

Verification:

```text
9e7b1390d7394ccbaddf0942b00085d859f8a0be
CMP Protocol Tests #3957 / SUCCESS
ESP32 Build #1751        / SUCCESS

8955f506f793b8a732a89726f38708ba2520945d
CMP Protocol Tests #3959 run 33254003888 / SUCCESS
```

No C++ source changed after `9e7b1390...` in checkpoint 159; later commits in that block are Web JS/test-only.

### Warehouse provenance suffix scan — GREEN

Checkpoint 161 keeps the full authoritative `movements.ndjson` transaction/schema/order pass, but removes duplicate provenance comparisons and repeated already-validated prefix reads.

```text
runtime dc9415c531d8c9685bc6202941df042ec299af0c
CMP Protocol Tests #3965 33257271690 / SUCCESS
ESP32 Build #1754        33257271722 / SUCCESS
Arduino RU LCD #178      33257271706 / SUCCESS
contract 875c6a069b3680569dc35576d82861d737444144
CMP Protocol Tests #3966 33257294547 / SUCCESS
```

Fixed `BatchSize=32` remains. Within-batch pairs are checked in RAM; cross-batch checks seek to `outer.position()` and scan only the later suffix, so each unordered provenance pair is checked once. Conflict semantics, RUN_WIRE provenance, recovery and mutation boundaries are unchanged.

Detailed record: `10_CHECKPOINT_161_WAREHOUSE_PROVENANCE_SUFFIX_SCAN.md`.

### Repair finalization known-repair proof reuse — GREEN

Checkpoint 162 closes the earlier finalization candidate without weakening generic callers.

Before finalization, both `RepairRegistryWeb` paths already execute authoritative:

```cpp
m_registry.repairIsOpen(repairId, repairOpen)
```

The new explicit `RepairFinalizationGuard::checkKnownRepair()` reuses that proof and calls existing `RepairCosting::loadKnownRepair()`. Generic `RepairFinalizationGuard::check()` still uses generic `RepairCosting::load()` and therefore remains self-validating.

`handleCloseRepair()` still performs the later mutation-time:

```cpp
m_registry.closeRepair(repairId, m_server.arg("closed_at"), alreadyClosed)
```

so the authoritative TOCTOU reread at the close mutation boundary remains intact.

Commits:

```text
6c3e967a5ca555f84bd5965e92ada22f8bc67bdd  known-repair guard API
cda4c10c1019681698facd64e1f0f1151c2adfff  generic/known internal implementation
6e104dfebc50464c8b6fc8bb39b17a7fe4a41d42  Web proof reuse
f61f7e17b227fd52e1e00a96c1f945ea1ac6749f  regression contract
```

Verification:

```text
ESP32 Build #1757         33257746469 / SUCCESS
Arduino RU LCD #181       33257746498 / SUCCESS
CMP Protocol Tests #3971  33257805004 / SUCCESS
```

`CMP #3970` (`33257746468`) failed only because the old source-text assertion expected the pre-refactor literal `if (!costing.load(repairId, summary))`; CMake/build/CTest 4/4 and all other checks passed. `f61f7e1...` updated the contract and #3971 is SUCCESS.

All costing, Warehouse transaction/provenance, material correction, winding transition and exact manual wire-writeoff coverage validation remains unchanged. No persistent cache/index/DB or unbounded state added.

Detailed record: `11_CHECKPOINT_162_REPAIR_FINALIZATION_KNOWN_REPAIR.md`.

## Current execution order

1. Continue only in `arduino-ru-lcd-experiment`; production stays at `28c7917...`.
2. Checkpoints 159–162 are closed; do not redesign them without a concrete regression.
3. Continue repo-reviewable repeated-scan/performance audit only for confirmed repeated growing-journal passes in the same request/operation.
4. Prefer an existing authoritative proof + explicit known/prevalidated read path over introducing persistent batch state.
5. Keep `MaterialUsageCorrectionIntegrityAudit` batch rereads **NO-CHANGE**: each batch must see previous corrections to prove cumulative over-correction limits.
6. Keep CashPayment correction preflight/fusion **NO-CHANGE** unless a simpler existing proof-preserving primitive appears; never weaken `append()` mutation-time authoritative scan.
7. Never remove recovery or mutation-time TOCTOU rereads solely for performance.
8. Preserve separate integrity domains; do not fuse unrelated ledgers only to reduce I/O.
9. Keep fixed RAM bounds: no whole-file buffering, unbounded vectors or caches on ESP32.
10. No automatic production rotation/deletion/truncation and no premature DB/index migration.
11. Verify actual CMP/ESP32/Arduino CI after each completed code block and update PROJECT_HANDOFF.
12. Full Arduino+ESP32 hardware E2E remains a separate final gate when hardware testing resumes.
13. Do not copy experiment commits into `cmp-protocol-v1` until separately requested.

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
