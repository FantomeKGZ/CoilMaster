# CoilMaster — калибровка датчика Холла SS49E

Дата: **2026-08-20**  
Source of truth: `cmp-protocol-v1`

## Текущее подключение и factory defaults

Arduino Uno:

```text
Hall SS49E -> A0
HallThreshold = 590
HallHysteresis = 50
release threshold = 540
```

Эти значения совпадают со старым рабочим скетчем пользователя, где наблюдалось примерно:

```text
без магнита ≈ 522
с магнитом ≈ 660
```

Это только baseline. Реальная установленная система должна калиброваться по live ADC.

## Почему нужна live-калибровка

SS49E аналоговый. Реальные значения зависят от:

- конкретного экземпляра датчика;
- питания 5 V;
- расстояния до магнита;
- положения/полярности магнита;
- механической вибрации;
- наводок от двигателя/SSR/проводки;
- температуры и разброса компонентов.

Поэтому оператор должен видеть фактический сигнал, а не вводить порог вслепую.

## Что должна показывать страница настроек

`Настройки -> Оборудование -> Датчик Холла`:

```text
Текущий ADC
Минимум за окно
Максимум за окно
Текущий threshold
Текущий hysteresis
Release threshold
Release debounce ms
Магнит: обнаружен / нет
Re-arm: armed / waiting release / release debounce
Возраст последнего sample
```

## Процедура калибровки

### 1. Снять уровень покоя

Магнит убрать из зоны датчика и нажать `Зафиксировать покой`.
Система собирает bounded sample window и показывает:

```text
idle_min
idle_max
noise_span = idle_max - idle_min
```

### 2. Снять уровень магнита

Разместить магнит так, как он реально проходит при работе, и нажать `Зафиксировать магнит`.
Показать:

```text
magnet_min
magnet_max
noise_span
```

### 3. Предложить порог

Для текущей rising-polarity установки, если:

```text
idle_max < magnet_min
```

можно предложить threshold около середины свободного диапазона:

```text
recommended ≈ (idle_max + magnet_min) / 2
```

Но UI НЕ сохраняет его автоматически. Оператор подтверждает вручную.

Если диапазоны перекрываются, показать:

```text
Калибровка ненадёжна: диапазоны покоя и магнита пересекаются.
Проверьте положение магнита, питание, проводку и шум.
```

## Защита от ложного повторного счёта

Один магнит, остающийся над датчиком, должен давать максимум один виток до реального ухода из зоны.

Текущая защита использует:

1. threshold crossing для первого импульса;
2. hysteresis: повторное разрешение только ниже `threshold - hysteresis`;
3. stable release debounce: сигнал должен оставаться ниже release threshold непрерывно заданное время;
4. если сигнал снова поднялся во время debounce, re-arm отменяется и отсчёт release начинается заново.

Текущий planned/default release debounce: **25 ms**, подтвердить hardware test перед окончательной фиксацией.

## Обязательные hardware tests

```text
A. Магнит неподвижно над датчиком -> максимум 1 импульс.
B. Магнит убрать и вернуть -> ровно следующий импульс.
C. Магнит уже над датчиком при START -> не должно быть бесконечного счёта.
D. Лёгкая вибрация магнита около точки срабатывания -> нет серии ложных импульсов.
E. Реальный оборот на рабочей скорости -> каждый оборот считается один раз, пропусков нет.
F. После reboot сохранённые настройки загружаются корректно.
G. Повреждённые settings -> fallback на factory defaults + явная диагностика.
```

## Persistence

Настройки Hall должны храниться на Arduino в EEPROM, потому что Arduino отвечает за real-time counting и должна работать корректно даже без ESP32.

Минимум сохранять:

```text
threshold
hysteresis
release_debounce_ms
schema_version
CRC
```

Запись должна быть recoverable/fail-safe. Invalid persisted data -> безопасные defaults `590/50`.

## Web/ESP32 telemetry

Live telemetry включать только когда открыта страница калибровки:

- Arduino locally samples Hall as required by control loop;
- telemetry наружу агрегировать примерно 5-10 Hz;
- ESP32 кэширует последний sample;
- browser обновляет экран примерно 250-500 ms;
- после ухода со страницы telemetry отключается;
- calibration telemetry не должна блокировать winding loop или засорять UART.

## Safety

- изменение настроек разрешено только в safe idle;
- никакой calibration endpoint не выполняет START;
- никакой calibration endpoint не включает SSR;
- test UI не должен имитировать физическую намотку;
- изменение параметров во время Winding/Paused/ManualRun запрещено fail-closed.

## Связанные файлы

```text
Arduino/Config/CM_Pins.h
Arduino/CM_HallTurnSource.h
Arduino/CM_HallTurnSource.cpp
firmware/arduino/src/main.cpp
docs/PROJECT_HANDOFF/40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md
```
