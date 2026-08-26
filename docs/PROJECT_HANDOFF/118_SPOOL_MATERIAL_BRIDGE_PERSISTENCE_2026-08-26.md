# 118 — Spool Material Bridge Persistence — 2026-08-26

Status: GREEN

## Purpose
Close the first safe migration block between the physical spool warehouse domain and the generic MaterialLedger domain without weakening the existing exact-spool production contract.

## Implemented
- Added append-only `SpoolMaterialBridgeStore` at `/data/warehouse/spool-material-bridges.ndjson`.
- Bridge identity links exact physical `spool_id` to generic `warehouse_item_id` and records `CU|AL`, diameter and link timestamp.
- One physical `spool_id` can be linked only once; duplicate/malformed/conflicting records fail closed.
- Duplicate validation uses bounded batches of 24 spool IDs rather than an unbounded or per-row full-file rescan.
- Added read-only cross-reference integrity audit:
  - exact physical spool must exist;
  - wire type and diameter must match the physical spool;
  - exact MaterialLedger item must exist;
  - MaterialLedger unit must be `GRAM` for wire bridge compatibility.
- Added bridge file to warehouse backup/export whitelist and warehouse persistence metrics.
- Permanent regressions cover append-only behavior, bounded audit, cross-reference integrity, backup/export coverage, and preservation of the old write-off contract.

## Safety state
- Existing `/api/warehouse/write-offs` and exact-spool finalization contract are unchanged.
- No runtime HTTP/API writer for `SpoolMaterialBridgeStore` is registered yet.
- No automatic stock mutation is introduced.
- `RUN_COMPLETED` still does not deduct material.
- Material Request `RUN_WIRE` is not yet allowed to replace exact-spool write-off.

## Verification
- CMP Protocol Tests `#3425`, run `32939884633`: SUCCESS, all 67 steps.
- ESP32 Build `#1532`, run `32939884635`: SUCCESS.
- Earlier intermediate CMP failures in this block were regression-test corrections; the final source state is covered by the successful runs above.

## Next
Extend the existing generic MaterialLedger with backward-compatible structured wire metadata (`CU|AL` + diameter) so a bridge can be validated from authoritative catalog data before exposing any operator bridge-creation flow. Only after that should the migration proceed toward crash-safe Material Request `RUN_WIRE` accounting while preserving exact physical spool provenance.
