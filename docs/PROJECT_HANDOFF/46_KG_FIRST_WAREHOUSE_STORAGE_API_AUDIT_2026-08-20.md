# KG-first warehouse storage/API audit — 2026-08-20

## Scope

Pre-implementation audit required by the kg-first roadmap. Source of truth: `cmp-protocol-v1`.

## Current authoritative invariants

1. Manual wire write-off is currently represented by `ConfirmedSpoolWriteOff` and requires:
   - non-zero `spool_id`;
   - non-zero `repair_id`;
   - exact non-zero `source_session_id + source_run_id`;
   - `weight_before_g > weight_after_g`;
   - timestamp;
   - a completed source RUN.
2. `confirmSpoolWriteOff()` additionally requires an immutable job spool-selection snapshot for the source session and requires its `repair_id` and `spool_id` to match the request.
3. Duplicate write-off protection is source-run based: a second CONFIRMED write-off for the same exact `source_session_id + source_run_id` is rejected.
4. The warehouse journal is append-only and transaction-shaped for spool mutation:
   - PENDING record;
   - exact spool weight mutation;
   - matching CONFIRMED record;
   - or ABORTED after successful rollback.
5. `WarehouseMovementIntegrityAudit` and `CM_WarehouseWriteOffHistory` currently hard-require non-zero `spool_id`, positive before-weight, lower after-weight and `mass_g == weight_before_g - weight_after_g` for every write-off record.
6. Finalization is source-run coverage based, but `WireWriteOffCoverageAudit` currently resolves coverage through the session spool-selection snapshot and requires the CONFIRMED movement to carry the same exact spool.
7. `RUN_COMPLETED` is only evidence that a run is eligible for a later manual write-off. It does not and must not trigger stock mutation or automatic consumption.

## Migration implication

A safe kg-first implementation cannot be a POST-only change. Adding a spool-less record without updating the authoritative validators would make the movement journal fail closed and could block costing/finalization.

The compatible target is a second manual write-off shape in the same append-only journal:

- `quantity_kg` is the operator-authoritative consumed quantity; an integral `mass_g` may remain as the internal/accounting representation derived deterministically from it;
- exact `source_session_id + source_run_id` remains mandatory and unique;
- material/conductor identity is snapshotted in the record;
- `spool_id` is optional only for the new kg-first mode;
- with `spool_id`: validate the exact spool and perform the existing guarded stock mutation path;
- without `spool_id`: persist the manual consumption record but do not mutate any spool weight;
- legacy spool write-offs remain readable, auditable and append-only with their existing semantics.

## Required coordinated change set

The first implementation increment must treat these as one compatibility boundary:

- `CM_WarehouseStore.h` record/API contract;
- `CM_WarehouseWriteOff.cpp` persistence and stock mutation;
- `CM_WarehouseWriteOffWeb.cpp` request validation;
- `CM_WarehouseMovementIntegrityAudit.cpp` dual-shape validation;
- `CM_WarehouseWriteOffHistory.cpp` dual-shape read/history output;
- `CM_WireWriteOffCoverageAudit.cpp` source-run coverage without mandatory spool for kg-first records;
- recovery logic for any transaction shape that can leave PENDING state;
- static regression contract(s).

Do not weaken legacy exact-spool checks while adding the new mode.

## Out of scope for this migration

- no automatic physical START;
- no auto-resume after reboot;
- no ESP32/Web direct SSR control;
- no automatic material deduction on `RUN_COMPLETED`;
- no rewrite/migration of existing NDJSON records;
- no premature database migration.

## Next implementation step

Introduce a backward-compatible explicit kg-first manual-consumption contract, preserving the legacy exact-spool write-off path unchanged, then update all authoritative readers/audits before exposing the mode to production UI.
