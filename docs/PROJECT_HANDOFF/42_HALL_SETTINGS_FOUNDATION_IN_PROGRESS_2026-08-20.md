# CoilMaster — Hall settings foundation verified

Дата: **2026-08-20**  
Ветка: **`cmp-protocol-v1`**

## Статус

Arduino Hall/settings foundation завершён и подтверждён сборкой.

Текущий кодовый foundation включает:

```text
Arduino/CM_HardwareSettings.h
Arduino/CM_HardwareSettings.cpp
Arduino/CM_HardwareSettingsController.h
Arduino/CM_HardwareSettingsController.cpp
Arduino/CM_HallTelemetry.h
Arduino/CM_HallTelemetry.cpp
Arduino/CM_HardwareControlProtocol.h
Arduino/CM_HardwareControlProtocol.cpp
Arduino/CM_HallTurnSource.h
Arduino/CM_HallTurnSource.cpp
Arduino/CM_UartEventTransport.h
Arduino/CM_UartEventTransport.cpp
firmware/arduino/src/main.cpp
```

## Persistent hardware settings

Factory defaults:

```text
hall_threshold = 590
hall_hysteresis = 50
hall_release_debounce_ms = 25
hall_direction = RISING
```

Настройки валидируются bounded-правилами и хранятся в двух слотах EEPROM с:

- magic/version;
- monotonic sequence;
- CRC;
- alternate-slot write;
- readback verification;
- выбором новейшего валидного слота;
- fallback на factory defaults, если оба слота невалидны.

### EEPROM layout audit

Проверены текущие `CM_EepromPersistence.h/.cpp`.
Существующий allocator + pending `RUN_COMPLETED` evidence используют начало EEPROM и metadata sidecar.
Новый `HardwareSettingsStore` резервирует два небольших слота только в конце EEPROM.
Пересечения адресных областей нет.

## Safe settings controller

`CM_HardwareSettingsController`:

- загружает settings store;
- применяет threshold/hysteresis/release debounce/direction к Hall driver;
- блокирует изменения в `Winding`, `Paused`, `ManualRun`, `CoilComplete`, `Fault`;
- возвращает явные результаты `Applied / Invalid / Busy / PersistenceFailed`;
- выполняет factory reset только через тот же safe-idle gate.

## Hall driver hardening

Stable-release защита обязательна:

```text
081b3ed1dc3849ec8b0c6898fd841acb6d5f2d76
fix: debounce Hall sensor release before rearming

2b14a91c5a90d8fd2d694c5c54308f47fcfea0b4
fix: require stable Hall release before next turn
```

Driver поддерживает:

- `RISING`: магнит активен при `ADC >= threshold`, release при `ADC <= threshold-hysteresis`;
- `FALLING`: магнит активен при `ADC <= threshold`, release при `ADC >= threshold+hysteresis`;
- stable release debounce симметричен для обоих направлений;
- при `reset()` магнит, уже находящийся в active-zone, считается обнаруженным и не должен давать бесконечный счёт;
- наружу доступны `rawValue`, `magnetDetected`, direction-aware `releaseBoundary`;
- наружу доступен `HallRearmState`:
  - `Armed`;
  - `WaitingRelease`;
  - `ReleaseDebounce`.

## Live Hall telemetry

Добавлен bounded `CM_HallTelemetry`:

- ADC sample примерно 20 Гц;
- наружу snapshot ограничен примерно 4 Гц;
- snapshot содержит raw/min/max, threshold, hysteresis, release boundary, release debounce, direction, magnet-detected, re-arm state, sample count и captured-at;
- telemetry не создаёт winding RUN;
- при переходе в физически активные состояния telemetry автоматически выключается.

## CMP1 hardware-control protocol

Все hardware-control кадры защищены CMP1 CRC.

ESP32 → Arduino:

```text
CMP1|CFG_GET|HALL|C|<CRC>
CMP1|CFG_SET|HALL|<threshold>|<hysteresis>|<debounce_ms>|RISING|C|<CRC>
CMP1|CFG_SET|HALL|<threshold>|<hysteresis>|<debounce_ms>|FALLING|C|<CRC>
CMP1|CFG_RESET|HALL|C|<CRC>
CMP1|HALL_TELEM|START|C|<CRC>
CMP1|HALL_TELEM|STOP|C|<CRC>
```

Arduino → ESP32:

```text
CMP1|CFG_STATE|HALL|<threshold>|<hysteresis>|<debounce_ms>|<direction>|EEPROM|C|<CRC>
CMP1|CFG_STATE|HALL|<threshold>|<hysteresis>|<debounce_ms>|<direction>|FACTORY|C|<CRC>
CMP1|CFG_ACK|HALL|APPLIED|C|<CRC>
CMP1|CFG_NACK|HALL|BUSY|C|<CRC>
CMP1|CFG_NACK|HALL|INVALID|C|<CRC>
CMP1|CFG_NACK|HALL|PERSISTENCE_FAILED|C|<CRC>
CMP1|HALL_STATE|<raw>|<min>|<max>|<threshold>|<hysteresis>|<release>|<debounce>|<direction>|<magnet>|<rearm>|<samples>|<captured_ms>|C|<CRC>
```

Hardware-control parser/formatter отделён от winding JOB/EVT transport, при этом используется тот же физический UART и тот же CRC алгоритм.

## Composition/runtime integration

`firmware/arduino/src/main.cpp` интегрирован с новым foundation:

```text
boot
→ HardwareSettingsController::begin
→ EEPROM settings или factory fallback
→ settings применяются к Hall driver
→ safe CFG read/write/reset
→ bounded Hall telemetry
→ auto-stop telemetry при physical-active state
```

Никакой hardware-control запрос не выполняет physical START и не включает SSR напрямую.

## Verification

Одноразовый verifier был переписан в verification-only режим и на текущем batch выполнил:

```text
pio run -e uno
pio run -e esp32
node Tests/Web/check_web_assets.js
node Tests/Web/check_release_contracts.js
node Tests/Web/check_final_acceptance_contracts.js
```

Success-path завершился коммитом:

```text
1c34efac17f8dd41f65f874052e9c693b9c3b048
chore: finalize verified Hall settings foundation
```

После полного успеха временные workflows `hall-settings-phase0-verify.yml` и старый `firmware-identity-finalize.yml` удалены.

Текущий статус:

```text
UNO BUILD: PASSED
ESP32 BUILD: PASSED
WEB CHECKS: PASSED
HARDWARE HALL TEST: PENDING
```

Build/check status относится именно к foundation до следующего ESP32 hardware-control/API изменения; после новых commits требуется новый verification pass.

## Следующий активный блок

```text
ESP32 hardware-control receive/transmit state
→ ESP32 cache/freshness/timeout
→ safe HTTP APIs
→ desktop/mobile Equipment page
→ manual live calibration
→ automatic calibration requiring physical START
```

После Hall/settings блока:

```text
repeat_target + final JOB clear
→ motor schema/UI
→ Arduino archive list/bulk linkage
→ kg-first material usage
→ shared web shell/clock/diagnostics
```

## Safety

Не менять:

- Web/ESP32 не выполняет physical START;
- Web/ESP32 не включает SSR напрямую;
- settings write разрешён только в proven safe idle;
- calibration rotation начинается только после physical START;
- calibration не создаёт RUN events;
- RUN_COMPLETED не списывает провод автоматически;
- persisted RUN evidence не перезаписывается settings storage;
- любой calibration abort/error обязан привести к SSR OFF.
