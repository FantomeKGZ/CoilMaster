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

## Ручная процедура калибровки

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

## Автоматическая калибровка — утверждено

Автоматическая калибровка должна быть основным рекомендуемым режимом, а ручная — дополнительным.

Критический safety flow:

```text
Web: Запустить автокалибровку
→ ESP32 только подготавливает calibration mode
→ Arduino проверяет safe idle
→ Arduino снимает 2–3 секунды idle ADC
→ LCD/Web: Нажмите физический START
→ оператор физически нажимает START
→ только Arduino включает SSR
→ Arduino вращает станок ограниченное время
→ Arduino сама выключает SSR
→ показывает измерения и рекомендации
→ оператор вручную выбирает Применить / Повторить / Оставить старые
```

Web/ESP32 не выполняют START и не включают SSR напрямую.

### Длительность

Начальный default:

```text
15 секунд
```

Опции:

```text
10 секунд — короткая
15 секунд — стандартная
30 секунд — расширенная
```

Предпочтительно комбинированное завершение:

```text
получено минимум 15 валидных проходов
ИЛИ достигнут timeout 30 секунд
```

Если данных достаточно раньше, калибровка может завершиться раньше.

### Какие данные собирать

До вращения:

```text
idle_min
idle_max
idle_average
idle_noise_span
```

Во время вращения:

```text
raw_min
raw_max
pulse_peak_min
pulse_peak_max
valid_pass_count
period_min_ms
period_max_ms
period_average_ms
signal_noise_span
```

Просто среднее ADC во время вращения не использовать как единственный критерий, потому что большую часть оборота магнит находится вне активной зоны.

### Определение направления сигнала

Автокалибровка должна уметь определить:

```text
RISING
FALLING
```

Текущий старый setup — RISING, где магнит повышал ADC.

### Ошибки автокалибровки

`STUCK_MAGNET` — сигнал остаётся в active-zone слишком долго.

```text
SSR OFF
CALIBRATION_ABORTED
```

`NO_MAGNET_SIGNAL` — после физического START нет валидных проходов в bounded interval.

```text
SSR OFF
CALIBRATION_ABORTED
```

`SIGNAL_UNSTABLE` / `CALIBRATION_UNRELIABLE` — noise слишком велик или idle/magnet ranges пересекаются.

```text
SSR OFF
ничего не сохранять автоматически
```

`USER_ABORT` — оператор отменяет калибровку физически; Arduino немедленно выключает SSR и не меняет сохранённые settings.

### Calibration state

Не смешивать calibration с winding JOB. Предусмотреть отдельные состояния/сервис, например:

```text
CalibrationIdle
CalibrationPreparing
CalibrationWaitingPhysicalStart
CalibrationRunning
CalibrationComplete
CalibrationAborted
```

Calibration mode не создаёт:

```text
RUN_STARTED
RUN_COMPLETED
completed_runs
wire writeoff
motor history
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
H. Web prepare автокалибровки не запускает двигатель.
I. Только physical START начинает calibration rotation.
J. Calibration не создаёт RUN_STARTED/RUN_COMPLETED.
K. SSR выключается при success, timeout, STUCK_MAGNET, NO_MAGNET_SIGNAL и USER_ABORT.
L. Ненадёжные измерения не применяются автоматически.
M. После Apply settings переживают reboot и обычная намотка продолжает работать.
```

## Persistence

Настройки Hall должны храниться на Arduino в EEPROM, потому что Arduino отвечает за real-time counting и должна работать корректно даже без ESP32.

Минимум сохранять:

```text
threshold
hysteresis
release_debounce_ms
signal_direction (если реализовано)
schema_version
CRC
```

Запись должна быть recoverable/fail-safe. Invalid persisted data -> безопасные defaults `590/50`.
После сохранения обязателен readback verify.

## Web/ESP32 telemetry

Live telemetry включать только когда открыта страница калибровки:

- Arduino locally samples Hall as required by control/calibration loop;
- telemetry наружу агрегировать примерно 5-10 Hz;
- ESP32 кэширует последний sample;
- browser обновляет экран примерно 250-500 ms;
- после ухода со страницы telemetry отключается;
- calibration telemetry не должна блокировать winding loop или засорять UART.

Во время автокалибровки дополнительно показывать elapsed time, valid passes, idle/magnet ranges, noise, текущие и рекомендуемые параметры.

## Safety

- изменение настроек разрешено только в safe idle;
- Web calibration prepare не выполняет START;
- Web/ESP32 не включают SSR напрямую;
- реальное вращение начинается только после physical START;
- Arduino всегда сама выключает SSR по окончанию/ошибке/отмене;
- calibration не является winding JOB и не создаёт production RUN evidence;
- изменение параметров во время Winding/Paused/ManualRun запрещено fail-closed;
- рекомендации не сохраняются без явного действия `Применить`.

## Связанные файлы

```text
Arduino/Config/CM_Pins.h
Arduino/CM_HallTurnSource.h
Arduino/CM_HallTurnSource.cpp
firmware/arduino/src/main.cpp
docs/PROJECT_HANDOFF/40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md
docs/PROJECT_HANDOFF/41_HALL_AUTOCALIBRATION_ACCEPTED_2026-08-20.md
```
