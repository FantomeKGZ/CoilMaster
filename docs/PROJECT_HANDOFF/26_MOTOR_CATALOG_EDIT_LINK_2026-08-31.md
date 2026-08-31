# Motor catalog edit link — 2026-08-31

Branch: `arduino-ru-lcd-experiment`

## Change

Added an explicit `Редактировать` action to each motor card in both catalogs:

- `firmware/esp32/web/desktop/motors.html`
- `firmware/esp32/web/mobile/motors.html`

The link preserves the exact motor identity through `motor_id` and opens the existing editor:

- desktop: `/desktop/motor-edit.html?motor_id=<id>`
- mobile: `/mobile/motor-edit.html?motor_id=<id>`

The existing motor detail page already had an edit link; this checkpoint closes the missing edit entry point in the catalog cards themselves.

## Regression coverage

`Tests/Web/check_motor_edit_ui.js` now explicitly requires both desktop and mobile motor catalogs to contain:

- the exact `motor-edit.html?motor_id=` route;
- the visible `Редактировать` action.

Regression commit: `107ede71f49555519c414b1739b94fd59b823c88`.

## Verified CI

For the UI changes:

- CMP #4642 — `33352593117` — SUCCESS — desktop change `5aa90af98dfc309b68deab3cc04d9b6a29bb09ee`;
- CMP #4643 — `33352616704` — SUCCESS — mobile change `3b400765d88f00f1e828fb73f2c06e5fe96f3762`;
- Arduino RU LCD #244 — `33352593167` — SUCCESS;
- Arduino RU LCD #245 — `33352616752` — SUCCESS;
- ESP32 #1814 — `33352593088` — SUCCESS;
- ESP32 #1815 — `33352616695` — SUCCESS.

Documentation HEAD `be7f815205eb2d91cbe5ca55a612feb4601c92f1` was independently verified by CMP #4644 (`33352647659`) as completed/success.

The new regression-test commit and this documentation update require their own exact-head CI verification before declaring the newest branch HEAD GREEN.

## Safety / scope

- No backend contract changed.
- No winding/version history semantics changed.
- No START/SSR behavior changed.
- No wire write-off behavior changed.
- Production branch `cmp-protocol-v1` was not modified.

## Commits

- desktop catalog action: `5aa90af98dfc309b68deab3cc04d9b6a29bb09ee`
- mobile catalog action: `3b400765d88f00f1e828fb73f2c06e5fe96f3762`
- regression coverage: `107ede71f49555519c414b1739b94fd59b823c88`
