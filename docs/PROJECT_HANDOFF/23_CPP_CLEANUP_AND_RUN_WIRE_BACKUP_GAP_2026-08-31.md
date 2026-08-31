# Checkpoint 23 — C++ cleanup / RUN_WIRE backup integrity

Date: 2026-08-31

Branch: `arduino-ru-lcd-experiment`.
Production `cmp-protocol-v1` was not modified.

## Confirmed cleanup

`CM_WindingJournalHealth.{h,cpp}` was proven retired and removed at:

`0a675fa5009c4e9c3aa3369a5e9fc1e264996ef6`

Exact verification:
- CMP #4700 / run 33364286595 / SUCCESS
- ESP32 #1833 / run 33364286614 / SUCCESS
- Arduino RU LCD #264 / run 33364286586 / SUCCESS

## Pricing cleanup correction

Removing `CM_RepairPricingIntegrityAudit.{h,cpp}` was rejected by exact CI because `CM_MaterialPersistenceIntegrityAudit.cpp` still has a compile-time dependency through its standalone broad `check(fs::FS&)` path.

Failure evidence:
- CMP #4701 / run 33364528304 / FAILURE
- ESP32 #1834 / run 33364528283 / FAILURE
- Arduino RU LCD #265 / FAILURE

The pair was restored by non-force commit:

`a9a4fa9b5f87dea607b6c19d2bb7daa27f69afc9`

ESP32 recovery:
- ESP32 #1835 / run 33364936829 / SUCCESS

The pricing regression now follows the authoritative backup owner `CM_BackupBusinessDataIntegrityAudit` and its real metrics flow. Aligned test commit:

`1bf3c2955cfc895c8dba0e193442f953d1b9a523`

Exact verification:
- CMP #4704 / run 33365099826 / SUCCESS

## Confirmed RUN_WIRE backup gap

`RunWireAccountingIntegrityAudit` is valuable and must stay. It verifies exact completed RUN_WIRE accounting across request movement, immutable spool selection, spool/material bridge, material usage, warehouse CONFIRMED movement, repair/spool/session/run provenance, wire metadata, pricing and transaction identity.

Current legacy owner:

`WorkshopPersistenceIntegrityAudit::check() -> RunWireAccountingIntegrityAudit::check()`

Current composite backup uses scoped audits directly and does not call `WorkshopPersistenceIntegrityAudit`, so `BackupExportWeb::snapshotStabilityReason()` currently misses this RUN_WIRE cross-log audit.

## Next required step

Restore explicit `RunWireAccountingIntegrityAudit` ownership in the current backup snapshot stability path. Do not hide it inside unrelated material/warehouse audits solely to reduce diff size.

Preserve:
- manual RUN_WIRE writeoff only;
- exact `spool_id + source_session_id + source_run_id` provenance;
- no automatic deduction on `RUN_COMPLETED`;
- fail-closed pending/recovery semantics;
- mutation-time rereads / TOCTOU safety;
- no DB/index migration.

Only after direct backup ownership is restored and exact CMP + ESP32 are GREEN should the old broad standalone wrapper chain be reconsidered for removal.
