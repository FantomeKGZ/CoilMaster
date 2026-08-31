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

## Safety / scope

- No backend contract changed.
- No winding/version history semantics changed.
- No START/SSR behavior changed.
- No wire write-off behavior changed.
- Production branch `cmp-protocol-v1` was not modified.

## Commits

- desktop catalog action: `5aa90af98dfc309b68deab3cc04d9b6a29bb09ee`
- mobile catalog action: `3b400765d88f00f1e828fb73f2c06e5fe96f3762`
