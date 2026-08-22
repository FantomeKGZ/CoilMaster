# Arduino source audit checkpoint — 2026-08-22

Branch: `cmp-protocol-v1`

## Pre-audit verification baseline

Operator-provided GitHub Actions evidence after exact-run writeoff cleanup confirms:

- `0614dd5d2b560f673f3ad18cb1f051e9d237aae7` — `CMP Protocol Tests` GREEN;
- `42f75126b6c1549c9cd76607f72f28e25bed0bdb` — `CMP Protocol Tests` GREEN;
- `89cdcae838335100fd24367cb45598a8fa0b9763` — `ESP32 Build` GREEN;
- `89cdcae838335100fd24367cb45598a8fa0b9763` — `CMP Protocol Tests` GREEN.

The red ESP32 build on intermediate commit `f7e75e2da82ca711b0085d38781354321b2b2311` is retained as regression history: that commit removed the obsolete session-only declaration before its implementation was removed by `89cdcae...`. The resulting source state is verified by the later green ESP32 build.

## Arduino audit findings

### 1. LCD row-1 synchronization marker collision — FIXED

`Arduino/CM_Lcd1602View.cpp` always owns columns 13..15 of LCD row 1 for the synchronization marker (`OK`, `P<n>`, `E<n>`). Previous row-1 labels were built as if all 16 columns were available, so long labels were visibly overwritten by that marker.

Confirmed examples included `KOL-VO KATUSHEK?` and `RUCHNOY REZHIM`; the same ownership conflict could affect other row-1 labels as values were appended.

Fix:

- commit `69c43d0bd6e948fbcaabf6bbebeedd7551476940` — `Reserve LCD sync marker columns`;
- explicit `SyncMarkerStart = 13` and compile-time three-column ownership assertion;
- row-1 labels compacted to fit the operator-text area before the marker;
- marker indexing now uses the ownership constant instead of magic indices.

Regression coverage:

- `99cb0b6896e3fa3509a8fac3261d49cb69da0641` — `Add Arduino LCD layout contract`;
- `559d864a3a175a2456d2489f25261693a94eeacd` — `Run Arduino LCD layout contract`.

The new contract protects the final-three-column marker ownership and rejects the old full-width labels that caused the collision.

### 2. Duplicate EEPROM metadata write — FIXED

`EepromPersistence::addPendingCompleted(event, job)` previously called the event-only overload, which persisted both the core pending event and a default metadata sidecar, then immediately loaded and wrote that sidecar again with job metadata.

This was not a state-corruption bug, but it caused an unnecessary second EEPROM metadata write for every newly queued completed run.

Fix:

- commit `3df4c5b311b54e70482a41e6ee0bf9ad0c7158f2` — `Avoid duplicate EEPROM metadata write`;
- the overload now performs one core event persistence and one metadata persistence for the normal valid-job path;
- existing-run metadata updates remain supported;
- the previous fail-safe semantic is preserved: the completed-run delivery evidence is still persisted even if optional job metadata is unexpectedly invalid.

EEPROM schema/layout/version were not changed.

## Files inspected in the Arduino layer

Deep source/header review includes the following active owners and adjacent declarations:

- `CM_BuzzerService.*` — KEEP; non-blocking buzzer owner;
- `CM_DebouncedButton.*` — KEEP; physical button debounce/edge owner;
- `CM_EepromPersistence.*` — KEEP; pending RUN_COMPLETED delivery + allocator persistence; one wear optimization fixed above;
- `CM_HallCalibrationProtocol.*` — KEEP; CRC-protected explicit calibration commands;
- `CM_HallCalibrationService.*` — KEEP; calibration requires the explicit physical-start transition before motor permit;
- `CM_HallTelemetry.*` — KEEP; optional read-only Hall telemetry window;
- `CM_HallTurnSource.*` — KEEP; hysteresis/release debounce turn source;
- `CM_HardwareControlProtocol.*` — KEEP; strict Hall CFG/TEL parser/formatter;
- `CM_HardwareSettings.*` — KEEP; CRC-protected A/B EEPROM hardware settings;
- `CM_HardwareSettingsController.*` — KEEP; settings application owner;
- `CM_Lcd1602View.*` — KEEP; LCD renderer; marker collision fixed above;
- `CM_SsrController.*` — KEEP; Arduino-only physical SSR output owner;
- `CM_UartEventTransport.*` — KEEP; production CMP1 UART JOB/event transport;
- `Config/CM_Features.h` — KEEP; compile-time diagnostic/telemetry feature gates;
- pin configuration — checked with no confirmed pin conflict in the current Arduino production layout.

No further confirmed functional defect was found in the reviewed Hall telemetry, Hall re-arm/hysteresis, calibration protocol/service, settings persistence, or SSR owner during this pass.

## Safety review

The Arduino fixes above do not alter the production safety ownership:

- physical START remains local/operator initiated;
- no automatic START was introduced;
- no repeat auto-start was introduced;
- no reboot auto-resume was introduced;
- SSR remains Arduino-owned;
- ESP32/Web do not directly drive SSR;
- calibration motor permit is reached only through its explicit physical-start transition;
- RUN_COMPLETED persistence/replay does not perform material writeoff;
- writeoff remains explicit/manual and exact-run on ESP32.

## Verification state for the new Arduino edits

**PENDING fresh CI evidence.**

Do not describe commits `69c43d0...` through `3df4c5b...` as GREEN until the applicable current `CMP Protocol Tests` and Arduino/compile build evidence are actually observed. The earlier exact-run writeoff cleanup baseline above remains independently verified GREEN.

## Next audit order

1. close any remaining declaration-only Arduino review;
2. continue into `Core/` state-machine/job/event ownership;
3. audit production `Shared/CMP1Text/` contracts together with both UART endpoints;
4. treat old `Shared/Protocol/` as host-test/legacy binary protocol, not as the production UART source of truth;
5. then continue the larger `firmware/esp32/src/` owner sweep.
