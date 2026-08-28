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

Core flow закрыт и зафиксирован checkpoint 151:

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

`material_not_found -> catalog/create` остаётся только optional UX polish, не integrity blocker. Frequently-used material shortcut сознательно не добавлен: безопасного bounded aggregation API нет, а новый duplicated favourites state/full growing-NDJSON scan не оправдан.

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

Runtime/UI:

```text
104f319e1cb8e466a229c0d40876b35aac6ded86
ESP32 Build #1708   run 33167009269 / SUCCESS
Arduino RU LCD #132 run 33167009264 / SUCCESS
```

Final contract:

```text
17c7c43058802f44d5f0b26d0da301190e792ebd
CMP Protocol Tests #3885 run 33167039155 / SUCCESS
```

Semantics:

- max batch = 24 Material Request ids;
- one streamed request-journal scan + one streamed status-journal scan per bounded page;
- global request/transition ordering remains validated;
- exact per-request lifecycle chain remains fail-closed;
- browser prefetch cache is one-shot;
- no mutation/POST from prefetch;
- exact manual RUN_WIRE session/run/spool provenance is unchanged.

### Unified autonomous/Web completed-job archive lifecycle — GREEN

Checkpoint 153 closes the experiment-side completed Web/ESP32 job projection/linkage lifecycle without changing physical-run ownership or exact provenance.

Final runtime/backup HEAD before handoff update:

```text
a23d681a58670866f281191e7d1be6c5915d11f9
CMP Protocol Tests #3920 run 33174170271 / SUCCESS
ESP32 Build #1734         run 33174170232 / SUCCESS
Arduino RU LCD #158       run 33174170231 / SUCCESS
```

Semantics:

- long monotonic Arduino `session_id/run_id` remain authoritative 32-bit IDs for EEPROM recovery, UART and exact provenance;
- archive UI shows a short row number (`№1`, `№2`, ...) while exact Session/Run remain available in Info/details and are used for linkage;
- completed Web/ESP32 jobs are projected into the common autonomous archive as `ESP32_JOB` without copying or fabricating physical RUN evidence;
- local autonomous runs remain `ARDUINO_LOCAL`;
- `ESP32_JOB` exact Session/Run linkage is first-write-only: same motor/role replay is idempotent, different motor/role is rejected;
- duplicate exact linkage in persisted data is fail-closed;
- append-only `/data/autonomous-windings/web-assignments.ndjson` participates in autonomous archive integrity validation;
- `web-assignments.ndjson` is included in backup/restore whitelist as `autonomous-winding-web-assignments`;
- no reset/reuse of historical Session/Run counters, no weakening of recovery/provenance.

Intermediate CMP failures #3909-#3916 were contract-test corrections during implementation; the final branch state is covered by consecutive GREEN #3917-#3920 and final runtime builds above.

### RUN_WIRE exact immutable-spool lookup — GREEN

Checkpoint 154 removes the browser-side full active-spool catalogue scan for one immutable `spool_id`.

Runtime/UI:

```text
42ff9c2f8bbbc1d65a945ff019502299197e9b76
ESP32 Build #1736         run 33174983830 / SUCCESS
Arduino RU LCD #160       run 33174983809 / SUCCESS
```

Final contract:

```text
fd3428cf2c9ebdafe7a6dfe20206281701d43f94
CMP Protocol Tests #3926 run 33175377848 / SUCCESS
```

Semantics:

- RUN_WIRE `findActiveSpool(spool_id)` now uses one existing authoritative read-only `GET /api/warehouse/spools/by-id?spool_id=...` request;
- server resolves exact id through `loadActiveSpoolIdentity()` and validates exact returned identity;
- 404 remains fail-closed: immutable spool is unavailable, no alternate spool is inferred/substituted;
- browser no longer pages `/api/warehouse/spools?material=ALL&limit=32` across the active catalogue;
- exact `source_session_id + source_run_id + spool_id`, manual confirmation and dedicated warehouse mutation endpoint remain unchanged;
- regression contracts prohibit returning to catalogue paging for exact immutable-spool lookup.

CMP #3923-#3925 failures were stale/test-only assertions after replacing the old catalogue implementation; host CTest/runtime remained intact, and final CMP #3926 is GREEN.

### Material Request create repair scan reuse — GREEN

Checkpoint 155 removes one confirmed duplicate full `repairs.ndjson` pass from a single Material Request create request.

Runtime:

```text
28d4704819fc2e09443448a3766d91974431f136
CMP Protocol Tests #3930 run 33181705062 / SUCCESS
ESP32 Build #1738         run 33181705074 / SUCCESS
```

Final contract:

```text
06f814922c5e081059545e3c4389a83f543efba0
CMP Protocol Tests #3931 run 33181752412 / SUCCESS
```

Semantics:

- `POST /api/material-requests` still performs authoritative `loadRepairIdentity(repair_id)` against `repairs.ndjson` and preserves 404/fail-closed behavior;
- after that successful exact lookup, the same request now resolves only repair status instead of invoking `repairIsOpen()` and scanning `repairs.ndjson` a second time;
- generic `RepairRegistry::repairIsOpen()` keeps its original exact existence check for callers that have not already validated repair identity;
- `repair-status.ndjson` validation/duplicate-close fail-closed behavior is unchanged;
- Material Request ids, immutable client/motor provenance, status semantics and HTTP errors are unchanged;
- no cache/index/DB or unbounded state was added; safety/RUN_WIRE/warehouse mutation semantics are untouched;
- contract test prohibits returning to `repairIsOpen()` from the create handler after `loadRepairIdentity()`.

The repair-scoped Material Request list path was also checked in this block: it already performs one bounded request page scan and uses the existing batch status path, so no speculative change was made there.

### Material Request Warehouse known-request status reuse — GREEN

Checkpoint 156 removes the next confirmed duplicate full `material-requests.ndjson` pass from a fresh generic Material Request warehouse mutation.

Runtime code:

```text
e9756e7c176fac6030c6adea68519695804b47e3
CMP Protocol Tests #3934 run 33182699055 / SUCCESS
ESP32 Build #1740         run 33182699044 / SUCCESS
```

Final regression contract:

```text
59f20bc192bd09a5012441aafec84fe726bec2cc
CMP Protocol Tests #3935 run 33183113778 / SUCCESS
```

Semantics:

- fresh `MaterialRequestWarehouseCoordinator::buildPending()` first performs authoritative exact `requestMatchesRepair()` through `MaterialRequestStore::appendByIdJson()`;
- after that exact request + repair proof, the lifecycle gate scans only `material-request-status.ndjson` and no longer asks `MaterialRequestStatusStore::resolve()` to re-scan `material-requests.ndjson` for existence;
- status-only lookup still validates global monotonic transition ids, canonical transitions, timestamp bounds and the exact request lifecycle chain fail-closed;
- only `DRAFT`/`ISSUED` remain eligible for warehouse mutation;
- recovery intentionally remains stricter: `requestAllowsWarehouseMutation()` still uses the normal `m_statuses.resolve()` path and therefore revalidates request existence before replaying a movement-only pending transaction;
- no cache/index/DB, whole-file buffering or unbounded collection was added;
- movement-first WAL ordering, pending recovery, repair-open gate, authoritative material state, unit/cost conversion and mutation-time ledger checks are unchanged;
- RUN_WIRE continues through its dedicated exact-safe coordinator and is not weakened or merged with the generic warehouse path;
- regression contract explicitly prevents fresh `buildPending()` from returning to the general existence-scanning resolver while requiring recovery to retain it.

The adjacent Material Request status-transition Web flow was audited after this block. It directly delegates one transition request to `MaterialRequestStatusStore::transition()` and does not perform a separate exact-request read first, so there is no same-request duplicate request-journal pass to remove there.

## Current execution order

1. Continue only in `arduino-ru-lcd-experiment`.
2. Continue repo-reviewable repeated-scan/performance audit; do not reopen already accepted material accounting without a concrete requirement.
3. Check remaining Material Request movement/history, Repair and Warehouse exact-id/list/server flows one block at a time; change only a confirmed repeated growing-journal pass in the same request/operation.
4. Material Request create duplicate Repair scan is already removed by checkpoint 155; generic exact-existence guards for unrelated callers remain intact.
5. Fresh generic Material Request Warehouse duplicate request-journal scan is removed by checkpoint 156; recovery must keep its independent request-existence validation.
6. Material Request status transition was audited NO-CHANGE after checkpoint 156: it has no preceding duplicate exact-request lookup.
7. RUN_WIRE status prefetch remains bounded: max 24 ids/page and one batch status resolution; do not replace it with per-item server scans.
8. Prefer existing authoritative exact/batch APIs over new persistent state/indexes.
9. Preserve separate integrity domains; do not fuse unrelated ledgers only to reduce I/O.
10. Keep fixed RAM bounds: no whole-file buffering, unbounded vectors or caches on ESP32.
11. No automatic production rotation/deletion/truncation and no premature DB/index migration.
12. Verify actual CMP/ESP32 CI after each completed code block and update PROJECT_HANDOFF.
13. Full Arduino+ESP32 hardware E2E remains a separate final gate when hardware testing resumes.
14. Do not copy experiment commits into `cmp-protocol-v1` until separately requested.

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
