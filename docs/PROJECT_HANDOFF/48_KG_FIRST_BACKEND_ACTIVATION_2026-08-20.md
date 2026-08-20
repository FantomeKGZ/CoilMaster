# Checkpoint 48 — kg-first backend activation (2026-08-20)

## Scope completed

The warehouse wire write-off backend now supports an explicit `KG_FIRST` mode alongside the legacy exact-spool mode.

### Quantity contract

- Operator-facing `quantity_kg` is parsed without floating point through `CM_KgQuantity`.
- Accounting/storage resolution remains exact integer grams (`mass_g`).
- Canonical kg text is persisted for KG_FIRST records and must round-trip exactly to `mass_g`.

### Journal compatibility

- Legacy records remain detected by absence of `writeoff_mode` and keep their exact spool + before/after weight semantics.
- New records explicitly persist `writeoff_mode:"KG_FIRST"`.
- New stock modes:
  - `SPOOL`: exact selected spool is retained and stock is mutated with expected-before compare-and-swap semantics.
  - `UNALLOCATED`: spool and weight fields are absent; an immutable conductor snapshot (`diameter_hundredths_mm`, `wire_type`) is required and no spool stock is mutated.
- Exact `source_session_id + source_run_id` is mandatory for every KG_FIRST record.
- Confirmed source-run provenance stays unique across legacy and KG_FIRST records.

### Manual-only deduction invariants

- `RUN_COMPLETED` remains eligibility evidence only.
- No automatic write-off path was introduced.
- The POST endpoint still requires an explicit operator write-off request.
- Duplicate confirmed write-off for the same exact source run is rejected.

### Reboot recovery

- Legacy spool transaction recovery is retained.
- KG_FIRST `SPOOL` recovery confirms only when durable spool weight equals the expected after-state; unchanged before-state closes as ABORTED.
- KG_FIRST `UNALLOCATED` has no stock mutation. A dangling PENDING after reboot is always closed as ABORTED; recovery never invents a confirmed deduction.

### Finalization coverage

- Coverage remains anchored to exact `RUN_COMPLETED` session/run records.
- Legacy write-offs must still match the immutable selected spool.
- KG_FIRST `SPOOL` must still match the immutable selected spool.
- KG_FIRST `UNALLOCATED` may explicitly cover the exact run without pretending that warehouse stock was mutated.

## HTTP contract

`POST /api/warehouse/write-offs`

Legacy mode (no `writeoff_mode`) remains compatible with the existing exact-spool request.

KG_FIRST mode requires:

- `writeoff_mode=KG_FIRST`
- `repair_id`
- `source_session_id`
- `source_run_id`
- `quantity_kg`
- `timestamp`

Optional:

- `spool_id`

When `spool_id` is omitted, the request additionally requires:

- `diameter_hundredths_mm`
- `wire_type` = `CU` or `AL`

If `spool_id` is supplied, the store resolves the authoritative material identity from the ACTIVE spool and rejects mismatched client conductor hints.

## Relevant commits

- `0e6feae8` — kg-first contract test wired into protocol CI workflow.
- `37c9fd23` / `cf3bd4a5` — dual journal codec + integrity validation.
- `de2120ec` — history reader supports legacy and KG_FIRST records.
- `a7dafdf4` — public/private warehouse kg-first store contract.
- `f55cb924` — manual kg-first persistence and guarded stock mutation path.
- `0acf6b3b` — reboot recovery for both KG_FIRST stock modes.
- `db621e43` — finalization coverage accepts exact-run KG_FIRST without weakening legacy exact-spool checks.
- `d1ad4790` — explicit KG_FIRST HTTP POST activation.
- `5df46c39` — active kg-first contract test hardened.
- `e3c4932f` — final acceptance assertions aligned with kg-first while preserving manual-only deduction.

## Verification status

At this checkpoint GitHub connector exposes no combined status checks or workflow runs for the latest direct-push commit. Therefore current ESP32 build / CI success is **not confirmed**.

## Next work

1. Compile-safety / CI follow-up for the activated backend and fix any surfaced build/static-contract errors.
2. Update warehouse desktop/mobile UI to make `quantity_kg` the primary consumption input and make spool allocation optional.
3. Preserve legacy exact-spool visibility in history and expose KG_FIRST `stock_mode` / `quantity_kg` clearly in the operator UI.
4. Add focused runtime/fault-path tests for duplicate exact-run write-off, reboot between PENDING/CONFIRMED, spool compare-and-swap mismatch, and UNALLOCATED finalization coverage.
5. Continue final production polish only after the backend checks are confirmed.
