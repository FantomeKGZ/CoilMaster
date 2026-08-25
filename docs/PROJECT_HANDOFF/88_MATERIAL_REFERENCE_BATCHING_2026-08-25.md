# 88 — Material reference batching — 2026-08-25

## Scope

Stage-1 ESP32/storage performance cleanup after GREEN block 87.

`MaterialPersistenceIntegrityAudit` previously performed per-record reference scans:

- every `usage.ndjson` row fully scanned `materials.ndjson` for exact `material_id` and `repairs.ndjson` for exact `repair_id`;
- every `adjustments.ndjson` row fully scanned `materials.ndjson` for exact `material_id`.

That produced O(n*m)-style filesystem reopen/scan cost as material usage history grew.

## Change

Reference validation is now processed in bounded batches of 32 rows.

For usage:

- collect up to 32 material references;
- collect up to 32 repair references;
- scan each referenced file once per batch.

For adjustments:

- collect up to 32 material references;
- scan `materials.ndjson` once per batch.

The outer authoritative material/usage/adjustment parsers are unchanged in purpose and still validate complete flat JSON shape, monotonic IDs, arithmetic, currency, timestamps and comments.

## Preserved fail-closed semantics

The new `resolveExactReferences()` keeps the original identity guarantees:

- referenced ID must be non-zero;
- referenced file must exist and be readable;
- every non-empty reference-file row must expose a valid non-zero identity field;
- every requested reference must match exactly once;
- a second match for the same requested ID fails immediately;
- missing references fail.

This intentionally does not add an unbounded in-memory index or optimistic cache.

## Expected I/O reduction

For `N` usage rows:

- before: up to `2 * N` full reference-file scans;
- after: up to `2 * ceil(N / 32)` full reference-file scans.

For `N` adjustment rows:

- before: up to `N` full material catalog scans;
- after: up to `ceil(N / 32)` full material catalog scans.

## Safety / deliberate non-changes

No changes to:

- physical START or SSR authority;
- reboot resume behavior;
- UART/Hall runtime;
- exact spool selection;
- manual wire write-off provenance;
- material accounting formulas or persisted schemas;
- NDJSON rotation/cleanup policy.

No automatic cleanup, compaction or database migration was introduced.

## Regression / CI

- `Tests/Web/check_material_reference_batching.js`
- CMP workflow step: `Audit material reference batching contracts`

Required software verification:

- ESP32 Build SUCCESS on implementation commit `3de9576579074fb058f6f8a7a4c6a0a459a07d2f` or descendant;
- CMP Protocol Tests SUCCESS on CI commit `7f27083227cc35e94cd86505ae9d8ca4a3d7c1ab` or descendant;
- explicit `Audit material reference batching contracts` SUCCESS;
- existing material ledger, material backup scoped, final acceptance and kg-first audits remain SUCCESS.

Hardware testing is not required for this repo-only optimization block.
