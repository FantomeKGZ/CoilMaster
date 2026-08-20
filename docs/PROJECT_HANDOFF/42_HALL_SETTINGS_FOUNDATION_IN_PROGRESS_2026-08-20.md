# CoilMaster — Hall settings foundation in progress

Дата: **2026-08-20**  
Ветка: **`cmp-protocol-v1`**

## Статус

Начата реализация утверждённого блока Hall/settings/autocalibration из checkpoints `40` и `41`.

На текущем этапе уже добавлены новые Arduino-модули:

```text
Arduino/CM_HardwareSettings.h
Arduino/CM_HardwareSettings.cpp
```

Они вводят отдельное recoverable хранилище hardware settings на Arduino Uno.

## Реализованная модель настроек

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

Hardware settings storage размещается в конце EEPROM и проектируется отдельно от существующего `CM_EepromPersistence`, который сохраняет allocator/pending `RUN_COMPLETED` evidence.

## Commits этого этапа

```text
438f571dafb0e30471de1960a9a5142a4408bd4c
feat: add persistent Arduino hardware settings

22260155148d81db70ca00148b831721bab89397
feat: add persistent Arduino hardware settings
```

## Hall counting hardening, уже присутствующий в ветке

```text
081b3ed1dc3849ec8b0c6898fd841acb6d5f2d76
fix: debounce Hall sensor release before rearming

2b14a91c5a90d8fd2d694c5c54308f47fcfea0b4
fix: require stable Hall release before next turn
```

Один удерживаемый магнит должен re-arm следующий импульс только после устойчивого выхода ниже release threshold в течение `release_debounce_ms`.

## Verification в процессе

Создан временный one-shot verifier:

```text
.github/workflows/hall-settings-phase0-verify.yml
7272d902c7421df25a17120c979aa483f7b35f6c
```

Он должен:

1. интегрировать `CM_HardwareSettingsStore` в Arduino runtime;
2. загрузить Hall threshold/hysteresis/release debounce при boot;
3. закрыть ранее незавершённый firmware-identification source block ESP32;
4. выполнить `pio run -e uno`;
5. выполнить `pio run -e esp32`;
6. выполнить web asset/release/final-acceptance checks;
7. только после success удалить временные verifier workflow-файлы.

На момент записи этого checkpoint success-path **ещё не подтверждён**. Нельзя утверждать, что новый UNO/ESP32 build green, пока workflow не завершился или пока не выполнена эквивалентная фактическая проверка.

## Следующие изменения без изменения roadmap

После reconciliation/verifier:

```text
Hall settings load at Arduino boot
→ safe CFG read/write protocol Arduino↔ESP32
→ bounded live Hall telemetry
→ desktop/mobile Equipment settings
→ manual live calibration
→ automatic calibration requiring physical START
→ hardware regression
```

После Hall/settings блока продолжать:

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
- persisted RUN evidence не перезаписывается settings storage.
