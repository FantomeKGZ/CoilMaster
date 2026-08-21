# 55 — Warehouse + winding bounded scan hardening — 2026-08-21

Branch: `cmp-protocol-v1`

## Scope

This checkpoint reduces repeated full-file scans in critical warehouse/finalization paths without changing persisted formats, source-run provenance semantics, or safety boundaries.

## Closed changes

### Finalization wire coverage

`CM_WireWriteOffCoverageAudit.cpp` no longer scans `/data/warehouse/movements.ndjson` once per `RUN_COMPLETED` target.

Current strategy:

- winding history is read in bounded pages of 32 validated events;
- completed runs with immutable spool selections become `CoverageTarget` entries;
- at most 32 targets are held in fixed-size RAM;
- `movements.ndjson` is scanned once for the whole batch;
- legacy session-only write-off semantics remain compatible;
- legacy exact-run records still require exact selected spool;
- KG_FIRST SPOOL requires the immutable selected spool;
- KG_FIRST UNALLOCATED may cover the exact run without a spool mutation;
- duplicate or mismatched provenance remains fail-closed.

Commit:

`2c7fe4e5e3dde9b52da6d32eb10c74734e7d1ee6`

### Winding completion evidence

`WindingSessionCompletionAudit::check()` previously performed:

1. full `validateAll()` schema audit;
2. full transition audit;
3. another filtered history scan to find `RUN_COMPLETED`.

The third scan is removed.

`WindingJournalTransitionAudit` now has a read-only overload that records whether the requested session/run completed while performing the authoritative transition scan.

`validateAll()` is intentionally retained because transition validation does not cover the complete schema/linkage contract.

`runId == 0` remains the historical wildcard meaning "any completed run in this session".

Commits:

- `e30ebdce0bc87a8cc7980945ad39693a04dd7796`
- `21038a5dfd0c5235a49fd73c3c6657d55c686435`
- `e311ac8cf2ae7e770c61ecc279bd6bec8e9cb7af`
- `3ed71b775c8ef25650eac6f3c9120e73dc1ab26b`

### Contract protection

`Tests/Web/check_kg_first_material_contracts.js` now requires:

- batched warehouse finalization coverage;
- absence of the old per-run `confirmedWriteOffExists()` full scan;
- retained schema `validateAll()`;
- completion evidence collected inside transition validation;
- absence of the redundant third `appendHistoryJson()` completion scan.

Commits:

- `281477e85deeeaaa4a8919d5df7ccbd81e64b940`
- `71505663ff2bafc13dd27f7a011c97a9ed9a22c4`

## Safety invariants unchanged

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` does not automatically deduct wire;
- write-off remains explicit/manual and tied to exact source provenance;
- persisted warehouse and winding formats are unchanged;
- ambiguous/corrupt history remains fail-closed.

## Verification status

Repository/static contract changes are committed.

Do not mark ESP32 build or CI green unless a real result is available for the current HEAD.

## Next safe work

Continue reviewing remaining growing-file readers for repeated full scans. Prefer bounded batches and reuse of already-authoritative validation passes. Do not introduce destructive compaction, database migration, or arbitrary rotation thresholds without measured device data.
