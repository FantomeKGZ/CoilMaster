# CoilMaster — калибровка датчика Холла SS49E

Дата: **2026-08-20**  
Source of truth: `cmp-protocol-v1`

## Текущее подключение и factory defaults

Arduino Uno:

```text
Hall SS49E -> A0
HallThreshold = 590
HallHysteresis = 50
HallReleaseDebounceMs = 25
signal direction = RISING
release boundary = 540
```

Старый рабочий скетч пользователя давал ориентировочно:

```text
без магнита ≈ 522
с магнитом ≈ 660
```

Это только baseline. Реальная установленная система должна калиброваться по live ADC.

## Уже реализованный Hall driver foundation

Текущий `CM_HallTurnSource` поддерживает два направления:

```text
RISING:
active  -> ADC >= threshold
release -> ADC <= threshold - hysteresis

FALLING:
active  -> ADC <= threshold
release -> ADC >= threshold + hysteresis
```

Повторный импульс разрешается только после устойчивого выхода из active-zone в течение `release_debounce_ms`.
Один случайный шумовой sample не должен повторно re-arm счётчик.

Для диагностики наружу доступны:

```text
rawValue()
magnetDetected()
releaseBoundary()
inverted() / direction
rearmState()
```

`rearmState()` имеет понятные состояния:

```text
ARMED
WAITING_RELEASE
RELEASE_DEBOUNCE
```

Именно эти данные должны отображаться в live UI.

## Persistent settings foundation

Созданы:

```text
Arduino/CM_HardwareSettings.h/.cpp
Arduino/CM_HardwareSettingsController.h/.cpp
```

Планируемые/реализованные поля:

```text
threshold
hysteresis
release_debounce_ms
signal_direction
schema/version
CRC
```

Хранилище двухслотовое, в конце EEPROM, с alternate-slot write и readback verify.
Оно не пересекается с существующим allocator/pending RUN_COMPLETED persistence в начале EEPROM.
Invalid settings -> factory fallback `590/50/25/RISING`.

Контроллер блокирует изменение hardware settings при физически активных состояниях.

## Что должна показывать страница настроек

`Настройки -> Оборудование -> Датчик Холла`:

```text
Текущий ADC
Минимум за окно
Максимум за окно
Текущий threshold
Текущий hysteresis
Release boundary
Release debounce ms
Signal direction: RISING / FALLING
Магнит: обнаружен / нет
Re-arm: ARMED / WAITING_RELEASE / RELEASE_DEBOUNCE
Возраст последнего sample
```

## Ручная калибровка

1. Убрать магнит и нажать `Зафиксировать покой`.
2. Собрать bounded window:

```text
idle_min
idle_max
idle_average
idle_noise_span
```

3. Разместить магнит в реальной рабочей позиции и нажать `Зафиксировать магнит`.
4. Собрать:

```text
magnet_min
magnet_max
magnet_average
magnet_noise_span
```

5. Определить направление:

```text
magnet > idle -> RISING
magnet < idle -> FALLING
```

6. Если диапазоны разделены с достаточным safety margin — предложить midpoint threshold.
7. Если диапазоны пересекаются — `Калибровка ненадёжна` и automatic Apply запрещён.
8. Любая рекомендация сохраняется только после явного `Применить`.

## Автоматическая калибровка — утверждено

```text
Web: Запустить автокалибровку
→ ESP32 только подготавливает calibration mode
→ Arduino проверяет safe idle
→ Arduino снимает 2–3 секунды idle ADC
→ LCD/Web: Нажмите физический START
→ оператор физически нажимает START
→ только Arduino включает SSR
→ Arduino вращает станок bounded время/число проходов
→ Arduino сама выключает SSR
→ ESP32/Web показывает measured + recommended settings
→ оператор: Применить / Повторить / Оставить старые
```

Web/ESP32 не выполняют physical START и не включают SSR напрямую.

### Длительность

```text
10 секунд — короткая
15 секунд — стандартная/default
30 секунд — расширенная/max bounded interval
```

Предпочтительно завершить раньше при достаточной статистике, например `>=15` валидных проходов.

### Данные автокалибровки

До вращения:

```text
idle_min / idle_max / idle_average / idle_noise_span
```

Во время вращения:

```text
raw_min / raw_max
pulse_peak_min / pulse_peak_max
valid_pass_count
period_min_ms / period_max_ms / period_average_ms
signal_noise_span
```

Просто среднее ADC вращения не использовать как единственную основу порога.

### Ошибки/защиты

`STUCK_MAGNET` — active-zone удерживается ненормально долго.

`NO_MAGNET_SIGNAL` — после physical START нет валидных проходов за bounded interval.

`SIGNAL_UNSTABLE / CALIBRATION_UNRELIABLE` — noise велик или диапазоны пересекаются.

`USER_ABORT` — физическая отмена оператором.

Для всех abort/error paths:

```text
SSR OFF
settings unchanged
no RUN event
```

## Calibration state

Не смешивать с winding JOB:

```text
CalibrationIdle
CalibrationPreparing
CalibrationWaitingPhysicalStart
CalibrationRunning
CalibrationComplete
CalibrationAborted
```

Naming может уточняться, но семантика обязательна.
Calibration не создаёт:

```text
RUN_STARTED
RUN_COMPLETED
completed_runs
wire writeoff
motor history
```

## Live telemetry

Telemetry включать только на calibration/equipment page.
Arduino читает ADC локально с нужной control-loop частотой, но наружу передаёт bounded агрегированные данные примерно 5–10 Hz.
ESP32 кэширует последний telemetry snapshot, browser обновляет экран примерно каждые 250–500 ms.

Минимальный snapshot:

```text
raw_adc
window_min
window_max
threshold
hysteresis
release_boundary
release_debounce_ms
direction
magnet_detected
rearm_state
sample_age/state
```

Во время autocal дополнительно:

```text
elapsed_ms
valid_passes
idle range
pulse range
noise
recommended threshold/hysteresis/direction/debounce
```

## Обязательные hardware tests

```text
A. Магнит неподвижно над датчиком -> максимум 1 импульс.
B. Магнит убрать и вернуть -> ровно следующий импульс.
C. Магнит уже над датчиком при START -> нет бесконечного счёта.
D. Вибрация около границы -> нет серии ложных импульсов.
E. Реальный оборот -> один оборот = один импульс, без пропусков.
F. RISING и FALLING работают симметрично.
G. Settings переживают reboot.
H. Invalid settings -> factory fallback + явная диагностика.
I. Web prepare не запускает двигатель.
J. Только physical START начинает calibration rotation.
K. Calibration не создаёт RUN events.
L. SSR OFF при success/timeout/STUCK/NO_SIGNAL/USER_ABORT.
M. Unreliable result не применяется автоматически.
N. После Apply обычная намотка работает с новыми settings.
```

## Verification status

Код foundation менялся после предыдущих подтверждённых baseline builds. На текущем этапе:

```text
UNO BUILD: NOT YET CONFIRMED FOR CURRENT HALL SETTINGS BATCH
ESP32 BUILD: NOT YET CONFIRMED FOR CURRENT HALL SETTINGS BATCH
REAL HALL REGRESSION: PENDING
```

Не переносить старый green build на текущий HEAD без фактической проверки.

## Связанные файлы

```text
Arduino/Config/CM_Pins.h
Arduino/CM_HallTurnSource.h
Arduino/CM_HallTurnSource.cpp
Arduino/CM_HardwareSettings.h
Arduino/CM_HardwareSettings.cpp
Arduino/CM_HardwareSettingsController.h
Arduino/CM_HardwareSettingsController.cpp
firmware/arduino/src/main.cpp
docs/PROJECT_HANDOFF/40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md
docs/PROJECT_HANDOFF/41_HALL_AUTOCALIBRATION_ACCEPTED_2026-08-20.md
docs/PROJECT_HANDOFF/42_HALL_SETTINGS_FOUNDATION_IN_PROGRESS_2026-08-20.md
```
