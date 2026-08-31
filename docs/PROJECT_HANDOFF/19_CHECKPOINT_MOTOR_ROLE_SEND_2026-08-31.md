# Checkpoint 19 — Motor role → safe winding-job navigation (2026-08-31)

## User-visible request

From the motor card, add a convenient **«Отправить на станок»** action directly under the current WORKING and STARTING winding roles.

## Implemented behavior

Development branch only: `arduino-ru-lcd-experiment`.

A new display/navigation helper was added:

- `firmware/esp32/web/shared/motor-role-send.js`

Desktop motor details now load this helper:

- `firmware/esp32/web/desktop/motor-details.html`

The helper injects one **«Отправить на станок»** button under WORKING and one under STARTING.

The motor card does **not** create `/api/jobs` directly and does not send UART or touch SSR. It only resolves the exact repair context and navigates into the existing protected linked-job page:

- `/desktop/winding-job.html?repair_id=<exact>&role=working`
- `/desktop/winding-job.html?repair_id=<exact>&role=starting`

That existing page remains authoritative for repair↔motor validation, exact winding role/program/repeat reread, exact spool selection, ESP32 readiness, job persistence/UART, and physical START semantics.

## Fail-closed repair resolution

The convenience action reads `/api/motors/repairs` using bounded cursor pages (`limit=20`) and considers only non-CLOSED repairs.

- exactly one OPEN repair → navigate to the protected winding-job flow for that exact repair and selected role;
- zero OPEN repairs → block and tell the operator to create/open a repair first;
- more than one OPEN repair → do not guess; block automatic selection and direct the operator to choose the exact repair from the existing repair list below.

STARTING is also reread from `/api/motors/winding/latest` immediately before navigation; if STARTING is absent, the action is blocked instead of silently substituting WORKING.

## Safety invariants unchanged

- no automatic physical START;
- no auto-resume after reboot;
- Web/ESP32 does not directly control SSR from this action;
- motor card does not POST `/api/jobs`;
- exact repair_id + motor_id remains required by the existing linked winding-job flow;
- exact spool selection remains required there;
- `RUN_COMPLETED` does not write off wire automatically;
- manual RUN_WIRE provenance remains unchanged.

## Regression coverage

Updated:

- `Tests/Web/check_motor_details_ui.js`

The contract now checks that:

- the desktop motor card loads the helper;
- both role-card buttons are present through the shared helper;
- authoritative winding role and bounded repair reads are used;
- CLOSED repairs are excluded;
- zero/multiple OPEN repair cases fail closed;
- STARTING presence is authoritative;
- navigation carries exact `repair_id` + role;
- the helper cannot POST `/api/jobs` or control SSR/automatic START.

## Commits

- `f1ba6e9ff8644bc8c9dec7fe56bbc55146cb7458` — add safe motor role send-navigation helper.
- `5ac0001fde2caaa75f600283936bd56ac4995c11` — load helper from desktop motor details.
- `7c53fbc7e06714da686640219dcf04f9a98cbe5e` — regression guard.

## CI status at documentation time

Exact CMP run for `7c53fbc7e06714da686640219dcf04f9a98cbe5e`:

- `CMP Protocol Tests #4659`
- run `33357268349`
- observed status when checkpoint was written: `in_progress`

Do not call this checkpoint GREEN until an exact SUCCESS is observed for the relevant code/test HEAD. A later documentation commit is not automatically GREEN unless its own exact run succeeds.

Production `cmp-protocol-v1` was not modified.
