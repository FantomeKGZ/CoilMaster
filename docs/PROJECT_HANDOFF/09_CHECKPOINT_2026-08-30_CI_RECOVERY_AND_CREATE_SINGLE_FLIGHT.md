# Checkpoint — 2026-08-30 — CI recovery + create-form single-flight

Branch: `arduino-ru-lcd-experiment`

Production/source-of-truth remains `cmp-protocol-v1`; it was not modified in this block.

## CI failure series #4611–#4619

The CMP failure series beginning at #4611 was traced to stale Web text-contract expectations after Russian localization of `firmware/esp32/web/shared/settings-system-diagnostics.js`.

Runtime/protocol logic was not failing. In the failing runs:

- protocol CTest suite passed;
- Arduino JOB/parser/LCD contracts passed;
- job/recovery/writeoff/warehouse/material/Hall contracts passed;
- ESP32 and Arduino RU LCD builds continued to succeed;
- only `Audit web JavaScript and navigation` and `Audit NDJSON growth diagnostics` failed because they still searched for the former English/legacy wording around `production` data cleanup/rotation.

The stale test expectations were updated in:

- `Tests/Web/check_web_assets.js`
- `Tests/Web/check_ndjson_growth_diagnostics.js`

No Russian UI wording was rolled back.

## Create-form duplicate-submit integrity fixes

A bounded audit of create-only pages found real duplicate-create risk from fast double click/tap.

### Client create

Mobile `client-new.html` previously had no single-flight lock while desktop already had one.

Now mobile:

- ignores a second submit while the first request is active;
- disables the save button during the request;
- requires returned `client_id` before accepting success;
- matches desktop field length bounds.

Regression coverage is in `Tests/Web/check_client_crm_ui.js`.

### Motor create

Both desktop and mobile `motor-new.html` previously allowed overlapping similarity checks and repeated create POSTs, including repeated clicks on the explicit `create anyway` action.

Now both surfaces use separate bounded state guards for:

- similarity check in progress;
- actual create request in progress.

The save/confirm buttons are disabled while mutation is active and a returned `motor_id` is required.

### Material create

Both desktop and mobile `material-new.html` previously allowed repeated POST `/api/materials` from a double click/tap.

Now both surfaces:

- use a single-flight `sending` guard;
- disable the submit button during mutation;
- require returned `material_id`;
- restore the button in `finally` on error.

`Tests/Web/check_crud_page_separation.js` now locks the client/motor/material create single-flight contracts and still checks script syntax / CRUD page separation.

## Confirmed GREEN

Current confirmed code/test HEAD before this documentation commit:

`12df7eabaef2f86236cee7dd7080cab7a5ec3fd0`

Exact CI confirmation:

- CMP Protocol Tests #4624 — SUCCESS — run `33320524358`.

This run confirms recovery from the stale diagnostics contracts and the new create-form single-flight regression coverage.

Do not infer ESP32/Arduino GREEN for the documentation commit itself unless exact runs confirm it.

## Safety invariants unchanged

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` alone does not deduct wire;
- wire writeoff remains manual and bound to exact `spool_id`, `source_session_id`, `source_run_id`;
- production `cmp-protocol-v1` remains untouched.

## Next repo-reviewable step

Do not continue a broad cosmetic or repeated-scan sweep.

Next work should be one of:

1. a concrete reproducible functional defect in the `arduino-ru-lcd-experiment` UI/runtime flow;
2. a bounded audit of the remaining create-only entities only if they can create duplicate persistent records (for example repair/spool create), with no expansion into edit/list actions unless a real duplicate-mutation risk is demonstrated;
3. hardware E2E verification when requested/available.
