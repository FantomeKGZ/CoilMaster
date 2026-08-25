# 84 — Warehouse backup scoped audit

Date: 2026-08-25  
Branch: `cmp-protocol-v1`

## Context

Stage-1 NDJSON review found a proven duplicate full read in the deep-backup snapshot path:

1. `WarehousePersistenceIntegrityAudit::check(storage, metrics)` validated `spools.ndjson`, `price.ndjson` and also scanned `movements.ndjson` to resolve spool/repair references.
2. `CM_BackupExportWeb::snapshotStabilityReason()` immediately afterwards ran `WarehouseMovementIntegrityAudit::check(storage, recordCount)`, which is the authoritative transaction/provenance audit for movements and also produces the backup movement count.

This caused an avoidable extra full read of growing `movements.ndjson` during deep backup.

## Change

`WarehousePersistenceIntegrityAudit` now has split behavior without changing its public signatures:

- `check(storage)` remains the original broad standalone audit. It validates spool/price persistence and then calls `checkMovementReferences(storage)`.
- `check(storage, metrics)` is the scoped composite-backup path. It validates only spool and price persistence and returns their record counts.

The backup manifest already follows the metrics call with `WarehouseMovementIntegrityAudit::check(...)`, so movement transaction integrity remains fail-closed and authoritative there.

## Preserved invariants

No safety semantics were weakened:

- standalone warehouse persistence audit still checks movement references;
- backup still validates warehouse movements independently and fail-closed;
- transaction pairing/provenance checks remain in `WarehouseMovementIntegrityAudit`;
- exact spool/repair/source identities are unchanged;
- manual wire write-off remains manual;
- physical START/SSR/Hall/UART behavior is untouched.

## Commits

- `0aac127acd0a63c86a13474894e060bad688473b` — scope warehouse persistence metrics overload for composite backup
- `1a832d017f0f84a1f1aa7fb9fc16c9c33cc7e197` — regression protecting standalone-vs-scoped behavior
- `a5f549c9ecea5a41725cc8e85bb2ac59709c04f6` — CI step `Audit warehouse backup scoped contracts`

## Verification required

Fresh software CI only:

- ESP32 Build on `0aac127a...` or descendant;
- CMP Protocol Tests on `a5f549c9...` or descendant;
- specifically verify `Audit warehouse backup scoped contracts`, warehouse spool/legacy material, write-off fault, final acceptance, NDJSON growth diagnostics.

Hardware acceptance remains deferred until the software batch is complete.
