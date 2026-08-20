# Checkpoint 49 — kg-first write-off UI

Date: 2026-08-20
Branch: `cmp-protocol-v1`

## Status

The manual wire write-off UI is now aligned with the activated kg-first backend in both desktop and mobile variants.

## Operator flow

The write-off page no longer asks for `weight_before` / `weight_after` as the primary operator input. The primary field is now **Количество, кг** (`quantity_kg`) with up to three decimal digits, matching the exact integer-gram accounting boundary on ESP32.

For the next uncovered completed winding run the shared controller:

1. reads the repair lifecycle and blocks new writes for a closed/unverifiable repair;
2. scans winding history for `RUN_COMPLETED` using bounded pagination;
3. scans confirmed warehouse write-offs using bounded pagination;
4. chooses an exact uncovered `source_session_id + source_run_id`;
5. loads the immutable spool selection for that session;
6. resolves the immutable spool in the active warehouse catalog when possible;
7. performs no write until the operator explicitly submits the form.

## Allocation modes

### SPOOL

Available only when the immutable selected spool is still active and has a valid classified material (`CU` or `AL`).

The UI posts the immutable `spool_id` and the kg quantity. The ESP32 remains authoritative for the current spool state/material snapshot and performs the guarded stock mutation.

A legacy/unknown-material spool is deliberately not offered as a stock-backed write-off target because the backend cannot authoritatively commit it as an active wire identity.

### UNALLOCATED

Always available for a valid exact completed run. No `spool_id` is posted and no spool stock is mutated. The operator supplies the conductor material (`CU`/`AL`) and diameter snapshot.

This mode records the exact manual material consumption for costing/finalization without pretending that a specific warehouse spool was changed.

## History UI

The desktop/mobile history now supports both old and new records and displays:

- `LEGACY_SPOOL` vs `KG_FIRST`;
- `SPOOL` vs `UNALLOCATED`;
- canonical `quantity_kg` when present;
- nullable spool identity (`без бухты` for unallocated);
- before/after weights only for stock-backed records that actually contain them;
- exact source session/run provenance;
- server-derived material and value totals.

## Safety invariants retained

- `RUN_COMPLETED` is eligibility evidence only and never triggers automatic deduction.
- Every new write-off remains an explicit manual POST.
- Exact `source_session_id + source_run_id` are mandatory.
- Duplicate confirmed write-off coverage for one exact run remains rejected.
- A supplied spool must still match the immutable spool selection.
- ESP32/Web does not gain physical START or SSR authority.
- Legacy exact-spool journal records remain readable and strictly validated.

## Commits in this UI block

- `533b7502` — shared write-off controller migrated to kg-first.
- `14057a0f` — desktop write-off form migrated to kg-first.
- `70c65b85` — mobile write-off form migrated to kg-first.
- `281a2329` — kg-first desktop/mobile UI contract guards.
- `99a70aa4` — do not offer stock-backed mode for unusable legacy/unknown-material spool identity.

## Verification state

Static contract coverage is wired into the existing CMP protocol workflow. `Tests/Web/check_web_assets.js` also syntax-checks every shared JS file and embedded HTML script when the workflow executes.

At this checkpoint the GitHub connector reports no combined statuses and no workflow runs for HEAD `99a70aa4aea896bd9a007ccff2e94c05001b2463`; therefore CI/build must be treated as **not confirmed**.

## Next repo-reviewable priority

1. Audit costing/report consumers against mixed `LEGACY_SPOOL + KG_FIRST` history and ensure no consumer assumes `spool_id` or weight fields are always present.
2. Add explicit UI/backend regression coverage for mixed legacy + kg-first repair histories.
3. Then continue runtime/fault-path acceptance work and growing-NDJSON performance/rotation review without premature database migration.
