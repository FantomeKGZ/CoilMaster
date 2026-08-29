# Experiment checkpoint 167 — reachable Hall LCD states — GREEN

Branch: `arduino-ru-lcd-experiment`  
Date: 2026-08-30

## Finding

Checkpoint 166 corrected the text of the `WaitingLocalConfirm` LCD branch. A follow-up reachability audit showed that this display branch itself was dead code:

- `HallCalibrationService::arm()` keeps the internal legacy state `WaitingLocalConfirm`, but immediately publishes `s_displayState = HallCalibrationState::ArmedWaitingPhysicalStart` for the operator UI;
- the next `HallCalibrationService::update()` automatically calls `confirmLocal()` while the internal state is `WaitingLocalConfirm`;
- therefore `HallCalibrationService::displayState()` never needs a separate `WaitingLocalConfirm` LCD screen.

On the flash-constrained Uno, keeping a separate operator prompt for that unreachable display state only costs code space and creates a future risk of control-hint drift.

## Change

`Arduino/CM_Lcd1602View.cpp` now renders only reachable Hall operator states:

- `ArmedWaitingPhysicalStart` → `HALL TEST READY` / `A OR START`;
- `Running` → active countdown;
- `WaitingApplyConfirm` → `SAVE HALL CFG?` / `#=YES B=NO`.

The unreachable `WaitingLocalConfirm` LCD branch was removed.

No Hall state-machine transition was changed. No physical/local START rule was changed.

## Regression contract

`Tests/Protocol/check_arduino_lcd_contracts.js` now proves the relationship between display reachability and the Hall service:

- ARM publishes `ArmedWaitingPhysicalStart` as display state;
- the internal `WaitingLocalConfirm` state remains auto-promoted by `update()`;
- keypad `A` ownership remains in the Arduino entrypoint;
- `CM_Lcd1602View.cpp` must not regain a `WaitingLocalConfirm` branch or either obsolete local-confirm prompt.

## Commits

- runtime cleanup: `f842ebb07c5c7e86248b7f6bea4e5897f3cfc012`
- reachable-state contract: `d4e498dcc221b5f480a8b8ceb315923d7fcef2d8`

## CI

Runtime commit `f842ebb07c5c7e86248b7f6bea4e5897f3cfc012`:

- Arduino RU LCD Uno #174, run `33267808888` — **SUCCESS**
  - `Build Uno RU LCD profile` — **SUCCESS**
  - `Compare Uno resource usage` — **SUCCESS**
  - `Build ESP32 validation profile` — **SUCCESS**
  - `Compare ESP32 validation resource usage` — **SUCCESS**

The CMP run on this intermediate runtime commit used the previous checkpoint-166 contract and therefore failed because that old contract still required the just-removed prompt. This was a stale-test expectation, not a runtime build failure, and was corrected immediately in the next commit.

Final code + contract commit `d4e498dcc221b5f480a8b8ceb315923d7fcef2d8`:

- CMP Protocol Tests #4022, run `33267828055` — **SUCCESS**
  - `Audit Arduino LCD contracts` — **SUCCESS**

## Safety invariants

Unchanged:

- no automatic physical START;
- no auto-resume after reboot;
- Arduino remains the sole SSR owner;
- ESP32/Web do not gain direct SSR control;
- Hall ARM still does not itself energize the motor;
- RUN/material write-off semantics are untouched.

## NEXT

Continue repository-reviewable Hall/RU-LCD acceptance only for concrete reachable-state/operator/protocol defects. Full Arduino + ESP32 hardware E2E remains the final acceptance gate.
