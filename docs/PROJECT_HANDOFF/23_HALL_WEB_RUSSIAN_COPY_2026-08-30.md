# Hall Web Russian copy cleanup — 2026-08-30

Branch: `arduino-ru-lcd-experiment`

## Scope

This checkpoint is Web-only. No Arduino/Uno firmware, protocol, EEPROM schema, START handling or SSR authority changed.

The Hall settings pages were reviewed for user-visible English leftovers that are unnecessary in the Russian experiment UI. Technical identifiers that are useful as actual protocol/electrical terms remain unchanged (`ADC`, `START`, `SSR`, `RISING`, `FALLING`).

## Mobile

Commit:

```text
fa3743da3dc90ffd21af50857561142b8dde9c79
```

`firmware/esp32/web/mobile/settings-hall.html` now uses Russian user-facing copy for:

- page/header `Датчик Холла` instead of `Hall`;
- `Граница отпускания` instead of `Release`;
- reset confirmation for the Hall sensor settings;
- telemetry details: direction, re-arm state, sample count and debounce delay are described in Russian.

## Desktop

Commit:

```text
55b4f4f78a31947f856307e268509934e2e2f7be
```

`firmware/esp32/web/desktop/settings-hall.html` now uses Russian user-facing copy for:

- min/max display reset;
- telemetry details that previously used `re-arm / samples / debounce`;
- the calibration explanation that previously used `threshold / hysteresis / direction` as user-facing prose.

## Exact CI evidence

Final Web/runtime HEAD for this cleanup:

```text
55b4f4f78a31947f856307e268509934e2e2f7be
```

Confirmed:

```text
CMP Protocol Tests #4608
run 33319508425
completed / success
```

At the latest exact metadata checks:

```text
ESP32 Build #1796 / run 33319508451 / in_progress
Arduino RU LCD Build #225 / run 33319508433 / in_progress
```

Do not cite those two builds as GREEN unless later metadata confirms `completed/success`.

## Safety and memory impact

- no Uno flash/RAM growth;
- no ESP32 machine-state change;
- no automatic physical START;
- no auto-resume after reboot;
- no direct Web/ESP32 SSR control;
- Hall EEPROM apply still requires the Arduino-local confirmation path;
- existing Hall calibration identity/reconciliation semantics are unchanged.

Given the verified `uno_ru_lcd` headroom of only 808 bytes from checkpoint 166, further language cleanup should prefer Web/ESP32 copy and avoid Uno-side growth unless a concrete hardware/user-visible defect requires it.

## Next

Continue only with reachable, concrete RU-experiment/UI defects. Do not restart speculative repeated-scan optimization; the existing handoff already marks that class as exhausted without measured evidence.
