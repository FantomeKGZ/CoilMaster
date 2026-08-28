# Активная работа и следующие шаги

Дата обновления: **2026-08-28**  
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
- Repair close/finalization duplicate checks retained as TOCTOU boundary — NO-CHANGE;
- Repair lookup/list and Warehouse exact/list paths already bounded/one-pass per request — NO-CHANGE;
- MaterialLedger pre-read + mutation-time reread retained — NO-CHANGE;
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

Runtime verification for `bf529900a8211f0b9a920ec237942bae2f7093c5`:

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

- client balance still obtains repair ids only from bounded `RepairRegistry::appendRepairsPageJson()` client-filtered pages;
- after the first returned repair id, `RepairCosting::repairExists()` performs one authoritative full `repairs.ndjson` validation for the request;
- subsequent repair ids use `RepairCosting::loadKnownRepair()` and do not repeat the full repair-journal scan;
- generic `RepairCosting::load()` still always performs `repairExists()` before delegating;
- `savePricing()` still uses generic `load()`; mutation semantics are unchanged;
- known-repair path skips only the already-proven repair-existence scan; all warehouse movement, material usage/correction, pricing, currency, overflow and RUN_WIRE pending checks remain identical;
- no persistent cache/index/database or unbounded collection added.

Intermediate contract commit `6a5ecda6eb514f65152d4d6ff1b00a7f995f109d` produced CMP #3940 FAILURE only because two source-text assertions expected different local spelling (`repairIds[i]/found` instead of actual `repairIds[0]/repairFound`). Runtime was unchanged; `19bf0300...` corrected only the assertions and CMP #3941 is fully GREEN.

## Current execution order

1. Continue only in `arduino-ru-lcd-experiment`; production stays at `28c7917...`.
2. Continue repo-reviewable repeated-scan/performance audit only for confirmed repeated growing-journal passes in the same request/operation.
3. Next inspect client-balance/costing aggregation beyond the now-removed repeated `repairs.ndjson` validation. `loadKnownRepair()` still invokes full costing validation/aggregation per repair; change this only if a bounded batch primitive can preserve every existing journal validation and exact per-repair result without unbounded RAM.
4. Keep CashPayment correction preflight NO-CHANGE unless a simpler proof-preserving primitive appears; never weaken `append()` mutation-time authoritative scan.
5. Prefer existing authoritative exact/batch APIs over new persistent state/indexes.
6. Preserve separate integrity domains; do not fuse unrelated ledgers solely to reduce I/O.
7. Keep fixed RAM bounds: no whole-file buffering, unbounded vectors or caches on ESP32.
8. No automatic production rotation/deletion/truncation and no premature DB/index migration.
9. Verify actual CMP/ESP32/Arduino CI after each completed code block and update PROJECT_HANDOFF.
10. Full Arduino+ESP32 hardware E2E remains a separate final gate when hardware testing resumes.
11. Do not copy experiment commits into `cmp-protocol-v1` until separately requested.

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
