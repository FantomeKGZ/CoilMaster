# Experiment checkpoint 166 — Hall calibration LCD local-start hint — GREEN

Branch: `arduino-ru-lcd-experiment`  
Date: 2026-08-30

## Defect

With `CM_LCD_RU_EN`, `HallCalibrationState::WaitingLocalConfirm` displayed `#=OK B=CANCEL`, while the actual keypad owner in `processKeypad()` accepts only `A` to start the locally confirmed Hall calibration and aborts on every other key. Therefore pressing `#` cancelled calibration despite the LCD advertising it as OK.

## Fix

`Arduino/CM_Lcd1602View.cpp` now displays:

`A=START B=CANCEL`

Only the operator-facing hint changed. The Hall state machine, Arduino SSR ownership, physical/local START semantics, reboot behavior and ESP32/Web safety boundaries are unchanged.

## Regression contract

`Tests/Protocol/check_arduino_lcd_contracts.js` now locks:

- `A=START B=CANCEL` for local Hall confirmation;
- the `WaitingLocalConfirm` / `key == 'A'` ownership in the Arduino entrypoint;
- absence of the incorrect `#=OK B=CANCEL` prompt.

## Commits

- runtime fix: `9bb3f8bcb409e2536a222b624aeb8e2ad41d34b4`
- regression contract: `1258dcb5ad2ceff6d08f40e3ebfdcfae4b4701a1`

## CI

Runtime commit `9bb3f8bcb409e2536a222b624aeb8e2ad41d34b4`:

- CMP Protocol Tests #4019, run `33267611763` — **SUCCESS**
- Arduino RU LCD Uno #173, run `33267614323` — **SUCCESS**
  - `Build Uno RU LCD profile` — **SUCCESS**

Contract commit `1258dcb5ad2ceff6d08f40e3ebfdcfae4b4701a1`:

- CMP Protocol Tests #4020, run `33267636367` — **SUCCESS**
  - `Audit Arduino LCD contracts` — **SUCCESS**

## Safety invariants

- no automatic physical START;
- no auto-resume after reboot;
- Arduino remains the sole SSR owner;
- ESP32/Web do not gain direct SSR control;
- RUN/material write-off semantics are untouched.

## NEXT

Continue repository-reviewable Hall/RU-LCD acceptance and fix only concrete operator/protocol mismatches. Full two-board Arduino + ESP32 hardware E2E remains the final acceptance gate.
