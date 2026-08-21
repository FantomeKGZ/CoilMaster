# Checkpoint 57 — KG-first backup warehouse integrity

Date: 2026-08-21
Branch: `cmp-protocol-v1`

## Finding

The deep-backup warehouse persistence audit still used the pre-KG-first movement assumptions:

- every movement line was required to contain `spool_id`;
- every movement line performed a full exact lookup scan of `spools.ndjson`;
- every movement line performed a full exact lookup scan of `repairs.ndjson`.

That made a valid `KG_FIRST / UNALLOCATED` movement fail backup stability even though runtime movement integrity, history, costing and finalization already accepted the dual schema.

## Fix

`firmware/esp32/src/CM_WarehousePersistenceIntegrityAudit.cpp` now:

- parses movement records through authoritative `WarehouseWriteOffRecordCodec`;
- always validates exact-one `repair_id` reference;
- validates a spool reference only when `record.hasSpoolId` is true;
- therefore accepts valid `KG_FIRST / UNALLOCATED` records without inventing a spool;
- preserves exact spool reference validation for legacy and KG-first SPOOL records;
- resolves repair/spool references in bounded batches of 32 instead of rescanning both ledgers for every movement line.

The spool catalog and warehouse price persistence checks remain unchanged.

## Regression protection

`Tests/Web/check_kg_first_material_contracts.js` now requires the backup persistence audit to use:

- `CM_WarehouseWriteOffRecord.h`;
- `WarehouseWriteOffRecordCodec::parse`;
- optional `record.hasSpoolId` handling;
- bounded `ReferenceBatchSize = 32` reference resolution.

It also rejects return of the old per-record `idExists()` / mandatory-spool implementation.

## Safety/integrity semantics

Unchanged:

- `RUN_COMPLETED` does not deduct material;
- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web do not directly control SSR;
- write-off remains manual and exact-run provenance remains authoritative;
- legacy exact-spool movements remain valid;
- backup remains fail-closed on malformed records or missing/non-unique external references.

## Verification status

This checkpoint is source-reviewed and guarded by static contracts in the repository.

ESP32 build: **NOT CONFIRMED for this batch**.
GitHub CI: **NOT CONFIRMED unless an actual workflow/status result is visible**.

## Next repo-reviewable work

Continue backup integrity review for redundant scans, but keep independent audit responsibilities where combining them would weaken failure isolation. In particular, inspect session-directory/deep session persistence scans and backup business-data batching before considering any rotation or destructive compaction.
