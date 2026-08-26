# 139 — Finalization write-off first-batch fused audit — 2026-08-26

Branch: `cmp-protocol-v1`

## Scope

Remove one redundant full `/data/warehouse/movements.ndjson` scan from finalization write-off coverage without changing bounded RAM, persisted formats, exact-run provenance, or safety boundaries.

## Closed change

Checkpoint 55 intentionally established a bounded coverage strategy:

- winding history is processed in pages of at most 32 validated events;
- at most 32 completed-run coverage targets are retained in fixed RAM;
- warehouse movements are scanned once per target batch;
- exact session/run/spool provenance remains mandatory.

Before checkpoint 139, `WireWriteOffCoverageAudit::check()` additionally performed a standalone full `WarehouseMovementIntegrityAudit::check(storage)` before scanning the first coverage batch. The first batch therefore caused two full movement-journal reads.

Checkpoint 139 adds bounded `WarehouseMovementIntegrityAudit::checkCoverageBatch()` and accumulates exact coverage evidence during the existing authoritative movement pairing pass. That same pass still performs global confirmed provenance uniqueness validation.

Current strategy:

1. read one bounded winding-history page;
2. build up to 32 exact `(session_id, run_id, spool_id)` targets;
3. for the first batch, call `checkCoverageBatch()` so movement transaction pairing, exact coverage matching and global provenance validation share the authoritative audit pass;
4. subsequent winding pages retain the existing lightweight bounded `confirmedWriteOffBatch()` movement scan;
5. every target must be confirmed or finalization returns `WriteOffRequired`.

This removes one redundant full movement-journal pass per finalization audit while keeping RAM bounded.

## Exact coverage semantics preserved

- matching record must belong to the same repair;
- legacy spool records require exact selected spool and exact `source_run_id`;
- KG_FIRST SPOOL requires exact `source_session_id + source_run_id + spool_id`;
- historical KG_FIRST UNALLOCATED remains readable only when no spool is present;
- duplicate matching evidence fails closed;
- malformed PENDING/CONFIRMED pairing fails closed;
- global confirmed provenance conflicts still fail closed.

No persisted schema changed.

## Source / contracts

```text
final source     0e4896e0139ac8b7f79effb02d644e42dd057d22
final contract   1d78995c5cd203cbfadbaf67aa03b48a813a5ca0
```

Relevant files:

- `firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.h`
- `firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.cpp`
- `firmware/esp32/src/CM_WireWriteOffCoverageAudit.cpp`
- `Tests/Web/check_finalization_winding_single_pass.js`

## Verified CI

```text
ESP32 Build #1602          32979299677 / SUCCESS
CMP Protocol Tests #3635   32979340004 / SUCCESS
```

Intermediate CMP runs before contract-name alignment are not final checkpoint evidence.

## Safety invariants unchanged

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino remains the SSR owner;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` remains evidence only and never automatically deducts material;
- wire write-off remains explicit operator action tied to exact provenance;
- restore remains explicit/operator-only/transactional/fail-closed;
- no automatic production-data deletion, truncation or rotation.

## Next safe work

Continue auditing growing-file read paths for concrete redundant full scans. Preserve fixed-size batches and authoritative validation; do not replace bounded processing with unbounded RAM structures or premature database migration.
