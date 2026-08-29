# Checkpoint 165 — Repair delivery single-pass mutation scan

Date: 2026-08-29
Branch: `arduino-ru-lcd-experiment`
Status: GREEN

## Problem confirmed

`POST /api/repairs/delivery` performed two consecutive full authoritative reads of the growing `/data/workshop/repair-deliveries.ndjson` journal:

1. Web preflight `resolveByRepair(repairId, ...)` checked whether the repair was already delivered.
2. `RepairDeliveryStore::append()` immediately repeated the full journal scan in `prepareAppend()` to enforce uniqueness, validate integrity and allocate the next `delivery_id` before mutation.

The second scan is the required mutation-time/TOCTOU boundary and must remain authoritative. The first scan was redundant.

## Change

`RepairDeliveryStore` now exposes an append overload with an explicit `alreadyExists` result. The existing two-argument append API remains compatible and still reports duplicate delivery as failure to legacy callers.

The authoritative `prepareAppend()` scan now performs all of the following in one pass immediately before append:

- validates NDJSON framing and each delivery record;
- validates monotonic `delivery_id` order;
- validates required repair/client/motor/timestamp fields;
- detects duplicate/corrupt delivery rows for the same repair;
- reports a valid existing delivery through `alreadyExists=true` without writing;
- allocates the next `delivery_id` when no delivery exists.

`RepairDeliveryWeb::handleCreate()` no longer calls `resolveByRepair()` before append. It consumes the mutation-time `alreadyDelivered` result and preserves HTTP `409 {"error":"repair_already_delivered"}` semantics.

GET `/api/repairs/delivery` remains unchanged and continues to use `resolveByRepair()` because that is an independent read request, not a duplicate mutation preflight.

## Safety / integrity preserved

- no mutation-time authoritative scan was removed;
- no cache/index or persistent in-memory state was added;
- append-only delivery evidence remains append-only;
- exact repair identity and CLOSED-state checks remain before delivery creation;
- explicit `confirmed=true` remains required;
- delivery remains independent of cash balance;
- no automatic physical START, auto-resume, SSR authority, RUN_COMPLETED writeoff, or exact-spool provenance behavior changed.

## Commits

Runtime/API:

- `0f871270470decc7b5fd530e7d8eb908a9488a73` — `perf(esp32): expose delivery append conflict result`
- `b6eeb8499e7559b7cb61d8d5ee0c01f819bc9d27` — `perf(esp32): fuse repair delivery conflict scan`
- `6d26b1d97ed3b9bebd2439d3fdd02f92b8dd1918` — `perf(web): reuse delivery mutation conflict scan`

Regression contracts:

- `9801b50648fa2702f34aeb9e988d90d0f54417c3` — `test(web): guard repair delivery single-pass append`
- `3c8f5c757b288d5a8598bd99b8d7f3fb4c4ecc7f` — `test(web): accept delivery append conflict result`

## CI evidence

Runtime head `6d26b1d97ed3b9bebd2439d3fdd02f92b8dd1918`:

- ESP32 Build `#1775`, run `33265112221` — SUCCESS
- Arduino RU LCD Build `#199`, run `33265112192` — SUCCESS
- CMP Protocol Tests `#4007`, run `33265112200` — FAILURE only because the pre-existing CRM static contract still matched the old `prepareAppend` signature. CMake configure/build/test and the other contracts, including the finalization single-pass audit, passed.

Corrected contract head `3c8f5c757b288d5a8598bd99b8d7f3fb4c4ecc7f`:

- CMP Protocol Tests `#4009`, run `33265221419` — SUCCESS; its `host-tests` job completed successfully, including `Audit CRM backup and integrity coverage` and `Audit finalization costing single-pass contracts`.

No C++ runtime source changed after `6d26b1d9`, so ESP32 #1775 and Arduino #199 remain the compile evidence for the runtime change while CMP #4009 is the final corrected host/static-contract evidence.

## NO-CHANGE decision recorded before this checkpoint

`main.cpp::restoreLatestJobState()` was reviewed after checkpoint 164. `JobRecovery::evaluate()` validates the immutable job snapshot and `JobDisplayRecovery::load()` later rereads that same small immutable snapshot. This is intentionally left unchanged: eliminating that reread would require retaining additional full snapshot/cache state across layers for a negligible non-growing-file saving and could hide later on-disk corruption. The optimization audit remains focused on growing NDJSON journals and clearly redundant proof→generic-read patterns.

## Next

Continue the repeated-scan/performance audit on `arduino-ru-lcd-experiment`, looking only for another confirmed growing-journal duplicate within one request. Preserve all mutation-time TOCTOU rereads and existing safety invariants. Production `cmp-protocol-v1` remains unchanged.
