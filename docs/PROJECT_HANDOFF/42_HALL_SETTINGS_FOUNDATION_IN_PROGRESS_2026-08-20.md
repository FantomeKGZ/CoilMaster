# CoilMaster — Hall settings foundation in progress

Дата: **2026-08-20**  
Ветка: **`cmp-protocol-v1`**

## Статус

Реализация утверждённого Hall/settings/autocalibration блока из checkpoints `40` и `41` продолжается.

Текущий кодовый foundation уже включает:

```text
Arduino/CM_HardwareSettings.h
Arduino/CM_HardwareSettings.cpp
Arduino/CM_HardwareSettingsController.h
Arduino/CM_HardwareSettingsController.cpp
Arduino/CM_HallTurnSource.h
Arduino/CM_HallTurnSource.cpp
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

Добавлен `CM_HardwareSettingsController`.

Он:

- загружает settings store;
- применяет threshold/hysteresis/release debounce/direction к Hall driver;
- блокирует изменения в `Winding`, `Paused`, `ManualRun`, `CoilComplete`, `Fault`;
- возвращает явные результаты `Applied / Invalid / Busy / PersistenceFailed`;
- выполняет factory reset только через тот же safe-idle gate.

## Hall driver hardening

Ранее добавленная stable-release защита остаётся обязательной:

```text
081b3ed1dc3849ec8b0c6898fd841acb6d5f2d76
fix: debounce Hall sensor release before rearming

2b14a91c5a90d8fd2d694c5c54308f47fcfea0b4
fix: require stable Hall release before next turn
```

После этого driver расширен:

- `RISING`: магнит активен при `ADC >= threshold`, release при `ADC <= threshold-hysteresis`;
- `FALLING`: магнит активен при `ADC <= threshold`, release при `ADC >= threshold+hysteresis`;
- stable release debounce симметричен для обоих направлений;
- при `reset()` магнит, уже находящийся в active-zone, считается обнаруженным и не должен давать бесконечный счёт;
- наружу доступны `rawValue`, `magnetDetected`, direction-aware `releaseBoundary`;
- наружу доступен `HallRearmState`:
  - `Armed`;
  - `WaitingRelease`;
  - `ReleaseDebounce`.

Это состояние будет передаваться в live telemetry и показываться в Web.

## Commits этого этапа

```text
438f571dafb0e30471de1960a9a5142a4408bd4c
22260155148d81db70ca00148b831721bab89397
feat: add persistent Arduino hardware settings

8387e5444db491d56069e7015985e34faf608023
d7158087af030760783e2e71ae9bd1e6fc52d749
feat: add safe Arduino hardware settings controller

dda5e0d2700b05a837cd26007cfcc9a86e77b54c
e314abb488274d802cd85af90f69af6831d100bd
feat: support rising and falling Hall signals

a95120a1fb29388202a81a4ac253f3c1364b573d
feat: apply Hall signal direction from settings

276f032a77bd110268ec97da189268f0e1b3f326
01367522ee84039b35a7db2dfdbabca4e7d875a4
feat: expose Hall rearm state for diagnostics
```

## Composition/runtime integration

`firmware/arduino/src/main.cpp` ещё не считается интегрированным с новым settings controller на момент этого checkpoint.
Большой composition root изменять только после свежего полного fetch и с текущим blob SHA; не заменять файл по обрезанному содержимому.

Следующая связка:

```text
HardwareSettingsController::begin at boot
→ safe CFG read/write protocol
→ bounded Hall telemetry
→ ESP32 cache/API
→ desktop/mobile Equipment UI
→ manual calibration
→ automatic calibration with physical START
```

## Verification

Временный one-shot verifier был создан как:

```text
.github/workflows/hall-settings-phase0-verify.yml
7272d902c7421df25a17120c979aa483f7b35f6c
```

Его success-path пока фактически не подтверждён: workflow не self-delete, combined status не дал подтверждённого green результата, а Arduino composition root остался без интеграции.

Поэтому для текущего Hall/settings code batch:

```text
UNO BUILD: NOT YET CONFIRMED
ESP32 BUILD: NOT YET CONFIRMED
WEB CHECKS: NOT YET CONFIRMED FOR THIS BATCH
HARDWARE HALL TEST: PENDING
```

Старые успешные build/checkpoint результаты не переносить автоматически на новый код.

## Следующие изменения

Без остановки продолжать:

```text
bounded Hall telemetry module
→ safe settings/CAL protocol
→ Arduino composition integration
→ ESP32 cache + APIs
→ desktop/mobile Equipment page
→ manual live calibration
→ automatic calibration requiring physical START
→ build/tests/hardware regression
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
