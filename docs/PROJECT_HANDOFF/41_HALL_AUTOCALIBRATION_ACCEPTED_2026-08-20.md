# CoilMaster — Hall automatic calibration accepted plan

Дата: **2026-08-20**  
Ветка: **`cmp-protocol-v1`**  
Статус: **утверждено пользователем; реализовать как часть Hall/settings блока до большой переработки motors UI**

## Source of truth

Использовать только `FantomeKGZ/CoilMaster`, ветку `cmp-protocol-v1`. `main` не использовать как источник реализации.
Перед каждым изменением существующего файла заново fetch текущий blob SHA.

## Safety-инварианты

- Web/ESP32 не выполняют physical START;
- Web/ESP32 не включают SSR напрямую;
- автоматическая калибровка только подготавливает режим;
- вращение начинается только после физического START на Arduino;
- calibration mode не является winding JOB;
- calibration mode не создаёт `RUN_STARTED`, `RUN_COMPLETED`, material writeoff или motor history;
- при ошибке/timeout/stuck magnet Arduino обязана выключить SSR;
- settings сохраняются только после явного подтверждения оператора и только в safe idle.

## Утверждённый UX

На странице:

```text
Настройки → Оборудование → Датчик Холла
```

должны быть два режима:

1. `Автоматическая калибровка` — основной рекомендуемый режим;
2. `Ручная настройка` — live ADC + ручная фиксация покоя/магнита.

## Автоматическая калибровка — flow

```text
Web: Запустить автоматическую калибровку
→ ESP32 просит Arduino перейти в CALIBRATION_PREPARE
→ Arduino проверяет safe idle и отвечает READY
→ Arduino 2–3 секунды измеряет idle ADC без вращения
→ LCD/ESP32: Нажмите физический START
→ оператор нажимает START
→ Arduino сама включает SSR и входит в CALIBRATION_RUNNING
→ вращение ограничено временем и/или числом валидных проходов
→ Arduino сама выключает SSR
→ CALIBRATION_COMPLETE
→ ESP32 показывает измерения и рекомендуемые параметры
→ оператор выбирает Применить / Повторить / Оставить старые
```

## Длительность

Начальный default:

```text
standard calibration: 15 s
```

Предусмотреть варианты:

```text
10 s — короткая
15 s — стандартная
30 s — расширенная
```

Лучше завершать по комбинированному условию:

```text
minimum_valid_passes >= 15
OR timeout >= 30 s
```

Если достаточная статистика набрана раньше, остановить раньше.

## Что измерять

До вращения:

```text
idle_min
idle_max
idle_average
idle_median/noise estimate
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

Не использовать просто среднее ADC вращения как единственную основу порога: большую часть оборота магнит находится вне активной зоны.

## Расчёт рекомендуемых параметров

Для rising signal:

```text
idle_high < magnet_low
recommended_threshold ≈ midpoint(idle_high, magnet_low)
```

Добавить safety margin от обоих диапазонов.

Для falling signal — зеркальная логика. Автокалибровка должна уметь определить направление:

```text
RISING
FALLING
```

Если диапазоны перекрываются или separation недостаточна:

```text
CALIBRATION_UNRELIABLE
```

и автоматическое применение запрещено.

## Обязательные защиты

### STUCK_MAGNET

Если сигнал остаётся в active-zone слишком долго без нормального выхода:

```text
SSR OFF
CALIBRATION_ABORTED
reason=STUCK_MAGNET
```

### NO_MAGNET_SIGNAL

Если после физического START за bounded interval нет валидных проходов:

```text
SSR OFF
CALIBRATION_ABORTED
reason=NO_MAGNET_SIGNAL
```

### SIGNAL_UNSTABLE

Если noise слишком велик или idle/magnet ranges пересекаются:

```text
SSR OFF
CALIBRATION_COMPLETE
result=UNRELIABLE
```

Ничего не сохранять автоматически.

### USER_ABORT

Оператор должен иметь безопасную возможность остановить calibration физической кнопкой/клавишей. Arduino немедленно выключает SSR и не изменяет сохранённые настройки.

## Arduino states

Не смешивать с `MachineState::Winding` JOB lifecycle. Добавить отдельный calibration sub-state/service, например:

```text
CalibrationIdle
CalibrationPreparing
CalibrationWaitingPhysicalStart
CalibrationRunning
CalibrationComplete
CalibrationAborted
```

Реализация может выбрать другой naming, но семантика должна сохраняться.

## Telemetry

Во время открытой страницы calibration:

- Arduino может sample ADC с частотой, необходимой локальному алгоритму;
- наружу агрегировать примерно 5–10 Hz;
- ESP32 кэширует последний telemetry state;
- browser обновляет live UI примерно 250–500 ms;
- telemetry отключается после выхода со страницы/завершения режима;
- telemetry не должна блокировать winding/control loop.

Показывать:

```text
raw ADC
window min/max
idle range
magnet/pulse range
noise
valid passes
current calibration state
elapsed time
current/recommended threshold
current/recommended hysteresis
release threshold
release debounce
signal direction
```

## Persistence

После `Применить` сохранить на Arduino в recoverable EEPROM settings:

```text
threshold
hysteresis
release_debounce_ms
signal_direction (если реализовано)
schema_version
CRC
```

После записи обязателен readback verify. Invalid persistence → factory fallback + explicit diagnostics.

Factory baseline остаётся:

```text
A0
threshold 590
hysteresis 50
release threshold 540
```

Старый sketch давал ориентировочно idle ≈522 и magnet ≈660; реальные значения определяются calibration на установленном станке.

## Verification

Обязательные проверки:

```text
manual live ADC works
stationary magnet -> max one count until true release
autocal prepare from web does not start motor
only physical START begins calibration rotation
no RUN_STARTED/RUN_COMPLETED emitted during calibration
SSR OFF at normal finish
SSR OFF at stuck magnet/no signal/user abort
autocal generates recommendation from measured ranges
unreliable ranges are rejected
settings are changed only after explicit Apply
saved settings survive reboot
invalid EEPROM falls back safely
normal winding works after calibration
```

## Порядок в общем roadmap

Checkpoint `40` остаётся основным большим планом. Этот checkpoint уточняет Hall Phase 3:

```text
Phase 0 reconcile/build state
Phase 1 Hall counting correctness
Phase 2 persistent Arduino hardware settings + safe protocol
Phase 3A manual live calibration
Phase 3B automatic physical-run calibration (этот checkpoint)
Phase 4 repeat_target + final JOB clear
Phase 5+ motors/archive/material/web redesign
```

## Новый чат

Для продолжения сказать:

```text
Продолжаем CoilMaster, ветка cmp-protocol-v1.
Прочитай docs/PROJECT_HANDOFF/00_READ_FIRST.md,
40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md,
41_HALL_AUTOCALIBRATION_ACCEPTED_2026-08-20.md и
docs/HARDWARE_REFERENCE/07_HALL_SENSOR_CALIBRATION.md.
Автоматическая Hall calibration утверждена: Web только готовит режим,
вращение начинается только после физического START, Arduino сама выключает SSR,
calibration не создаёт RUN events и не меняет настройки без явного Apply.
```
