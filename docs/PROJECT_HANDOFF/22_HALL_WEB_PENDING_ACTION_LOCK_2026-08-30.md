# Hall Web pending-action lock — 2026-08-30

Branch: `arduino-ru-lcd-experiment`

## Finding

The shared Hall calibration Web controller left ARM / ABORT / APPLY buttons enabled until the first status poll after a POST (roughly 200–250 ms). A fast double-click could therefore submit the same operator action twice.

The ESP32/Arduino calibration state machine remained authoritative and fail-closed, so this was not a START/SSR or EEPROM safety bypass. It was still a real duplicate-submit/operator UX defect.

## Fix

Commit:

```text
957763c522bd7136d205f0ad1d921c22eec4d604
```

`firmware/esp32/web/shared/settings-hall-calibration.js` now immediately locks:

- ARM before `/calibration/arm` POST;
- ABORT before `/calibration/abort` POST;
- APPLY before `/calibration/apply` POST.

On transport/API failure the relevant control is restored or re-synchronized from authoritative state. On success the existing poll/state renderer remains the owner of subsequent button state.

Because this is the shared helper, desktop and mobile Hall settings receive the same behavior.

## Exact CI evidence

```text
CMP Protocol Tests #4605
run 33319349186
head 957763c522bd7136d205f0ad1d921c22eec4d604
completed / success
```

At the latest exact metadata check, ESP32 Build #1794 (`33319349203`) and Arduino RU LCD #223 (`33319349190`) were still reported `in_progress`; do not cite them as GREEN until later metadata says completed/success.

## NO-CHANGE follow-up

The rest of the Hall Web semantics were rechecked:

- desktop/mobile use the same validation ranges and shared calibration controller;
- physical START remains required locally on Arduino;
- Web never gets direct SSR authority;
- EEPROM apply still requires the Arduino-local `#` confirmation;
- browser visibility/pagehide stops browser polling/telemetry behavior but does not invent an automatic calibration resume/start;
- no additional large Hall firmware change is justified, especially with only 808 bytes of verified `uno_ru_lcd` flash headroom from checkpoint 166.

## Next

Continue only concrete Hall/RU experiment defects. Web-only localization cleanups are preferred over Uno-side growth when they do not alter machine semantics.
