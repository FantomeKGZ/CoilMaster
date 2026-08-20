# Hall auto-calibration checkpoint — 2026-08-20

Branch: `cmp-protocol-v1` only. `main` is not a source of truth.

## Goal

Add a safe Hall-sensor auto-calibration workflow that can be initiated from the ESP32 web UI without giving ESP32/Web any direct ability to start the motor or control SSR.

## Safety contract

Preserved invariants:

- no automatic physical START;
- ESP32/Web does not control SSR directly;
- after calibration ARM the operator must press the physical START button on Arduino;
- repeated physical START or keypad input during calibration aborts the procedure;
- calibration does not emit normal winding `RUN_STARTED/RUN_COMPLETED` evidence;
- calibration does not perform wire writeoff;
- calibration recommendations are not applied automatically;
- Hall settings are changed only by a separate explicit operator action.

## Arduino runtime

Implemented calibration state flow in Arduino runtime:

```text
IDLE
→ ARM command received
→ ARMED_WAITING_START
→ physical START
→ RUNNING
→ COMPLETED or ABORTED
```

The web/ESP32 command only arms the mode. Motor permit remains local to Arduino calibration state and physical START.

Calibration protocol responses:

```text
CMP1|CAL_STATE|<STATE>|baselineReady|motorPermit|C|CRC
CMP1|CAL_RESULT|VALID/INVALID|baseline|min|max|threshold|hysteresis|direction|samples|duration|C|CRC
```

Commands:

```text
CMP1|CAL|ARM|C|CRC
CMP1|CAL|ABORT|C|CRC
CMP1|CAL|GET|C|CRC
```

Relevant source commit:

```text
ee5a05e85290c109d16c445146075658af0a6e27
```

## ESP32 UART/control lane

Calibration is integrated into the existing `HardwareControlClient` used by `UartEventReceiver`.

There is no second UART reader. JOB/JOB_CANCEL and Hall service requests continue to share the same control lane so concurrent requests fail closed as busy.

Added:

- `HallCalibrationRemoteStateSnapshot`;
- `HallCalibrationRemoteResult`;
- ARM / ABORT / GET requests;
- strict CRC parsing of `CAL_STATE` and `CAL_RESULT`;
- cached state/result handoff through `UartEventReceiver`.

Commits:

```text
c2444364e58bdc7c8c79c0cb985961a490bca5b7
5a72c545e4856399fe8a3bd5c8b47caf584ab20a
450b0550dc73a890c087c126329dcd2ec8687138
a7c143121322373d55eb714f763ec2f10ab41c8a
```

## ESP32 HTTP API

Added to `CM_HardwareControlWeb`:

```text
GET  /api/hardware/hall/calibration
POST /api/hardware/hall/calibration/refresh
POST /api/hardware/hall/calibration/arm
POST /api/hardware/hall/calibration/abort
```

There is deliberately no calibration START HTTP endpoint.

Applying recommended settings still goes through the existing explicit Hall settings endpoint:

```text
POST /api/hardware/hall/settings
```

Commits:

```text
6a5eb1563443f302b2a96fcb8a78988fb86883f0
66b00e270d371dd7ddcb268560de1ce7d8e33061
```

## Web UI

New shared controller:

```text
firmware/esp32/web/shared/settings-hall-calibration.js
```

It is used by both:

```text
firmware/esp32/web/desktop/settings-hall.html
firmware/esp32/web/mobile/settings-hall.html
```

UI explicitly tells the operator:

```text
Нажмите физическую START на станке
```

Controls:

- `Вооружить калибровку`;
- `Прервать`;
- `Применить параметры`.

`Применить параметры` is a separate manual action. It applies only recommended threshold/hysteresis/direction through the normal Hall settings API; release debounce remains an explicit existing field.

Commits:

```text
cbccc79a0b98cd06ed08f16e4ed314806891d602
335f63b2606bcc2e05d6d0915cd54db99adb0357
282dcd1ed5455f916ab80c938ab2c0973731ffeb
```

## Contract test

Added:

```text
Tests/Web/check_hall_calibration_contracts.js
```

It checks, among other things:

- desktop/mobile use the shared controller;
- both UIs contain physical START wording;
- ARM/ABORT/refresh/settings paths exist;
- shared web controller has no `/start` or `/ssr` control path;
- ESP32 API has no `/api/hardware/hall/calibration/start` endpoint;
- ESP32 client uses CAL ARM/ABORT/GET and parses CAL_STATE/CAL_RESULT;
- Arduino runtime contains the physical-start calibration states.

The test is now included in `.github/workflows/cmp-protocol-tests.yml`.

Commits:

```text
335d81924f3d2ac72b8e8916b4f36278def3849f
d971cd85e32c0d2f9a3ab2d284af398d33e588ba
```

## Verification status

Do not claim this batch as build-verified yet.

Current facts:

```text
SOURCE IMPLEMENTATION: PRESENT
DESKTOP UI: PRESENT
MOBILE UI: PRESENT
CONTRACT TEST: PRESENT
LOCAL CLEAN CLONE/BUILD IN CHAT CONTAINER: NOT AVAILABLE (DNS blocked)
PUSH ACTION RUN RESULT: NOT AVAILABLE THROUGH CURRENT CONNECTOR VIEW
PIO UNO BUILD FOR THIS BATCH: NOT YET CONFIRMED
PIO ESP32 BUILD FOR THIS BATCH: NOT YET CONFIRMED
HARDWARE AUTO-CALIBRATION E2E: PENDING
```

## Required next verification

Run against the current `cmp-protocol-v1` HEAD:

```text
node Tests/Web/check_hall_calibration_contracts.js
node Tests/Web/check_web_assets.js
node Tests/Web/check_release_contracts.js
node Tests/Web/check_final_acceptance_contracts.js
pio run -e uno
pio run -e esp32
```

Then flash both boards and replace microSD `/web` with the current repo web tree.

Hardware test sequence:

```text
1. Open Hall settings page.
2. Press "Вооружить калибровку".
3. Confirm UI reports waiting for physical START.
4. Confirm motor has NOT started.
5. Press physical START on Arduino.
6. Observe calibration RUNNING state.
7. Confirm result appears after completion.
8. Confirm recommendation has not changed Hall settings automatically.
9. Press "Применить параметры" explicitly.
10. Refresh Hall settings and confirm applied values.
11. Repeat test with keypad/repeated START during calibration and confirm ABORT + motor stop.
12. Confirm normal winding JOB/RUN flow still works afterward.
```

Hardware E2E is mandatory before considering Hall auto-calibration production-complete.
