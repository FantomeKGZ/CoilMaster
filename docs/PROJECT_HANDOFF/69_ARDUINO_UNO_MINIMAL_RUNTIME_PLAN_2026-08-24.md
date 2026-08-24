# CoilMaster — план минимального runtime Arduino Uno

Дата: **2026-08-24**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Назначение

Этот файл — быстрый handoff в новый чат для продолжения аппаратного восстановления CoilMaster. Текущая цель: оставить Arduino Uno минимальным безопасным realtime-контроллером, вернуть проверенную клавиатуру и перенести расширенный тест/анализ Hall на ESP32 без передачи ESP32 права управлять SSR или автоматически запускать двигатель.

## Подтверждённая аппаратная картина

- LCD 1602 физически исправен: I2C scanner стабильно находил `0x27`, отдельный `LiquidCrystal_I2C(0x27,16,2)` test выводил текст.
- Production firmware первоначально показывала блоки, затем циклический `CM BOOT`.
- Изоляция внешних модулей не устранила reset-loop.
- USB diagnostic показал `M=0`; ELF-карта и PlatformIO подтвердили исчерпание SRAM.
- Исходный production baseline: RAM `1965/2048` (95.9%), Flash `31300/32256` (97.0%), только 83 байта static headroom.
- Крупнейшие ELF owners: `espTransport=355`, legacy `Keypad=111`, пять Wire/TWI buffers по 32 байта.
- После compact keypad + streaming winding UART: RAM `1770/2048` (86.4%), Flash `31022/32256` (96.2%), 278 байт static headroom.
- Production теперь запускается и не перезапускается.
- USB diagnostic проходит `SERIAL/LCD/UART/EEPROM/HW_SETTINGS/SSR_SAFE_OFF/BUZZER/KEYPAD/START_BUTTON/STATE/OUTPUTS/READY`.
- Текущий compact keypad физически не реагирует ни на одну клавишу даже после смены направления scan. До удаления библиотеки Keypad клавиатура работала.
- Пользователь подтвердил: до добавления расширенного Hall test/calibration блока система работала нормально.

## Последние релевантные коммиты

```text
14209cca  fix(arduino): initialize LCD before startup services
4bcb43b6  fix(arduino): expose reset loop on LCD
24685cbe  fix(arduino): retain last loop phase across resets
951314d9  build(arduino): add USB diagnostic environment
6413dfe7  build(arduino): use lightweight reset diagnostics
358558b6  refactor(arduino): replace Keypad library with compact scanner
7437c303  build(arduino): remove obsolete Keypad dependency
ae61d39c  refactor(arduino): stream winding UART frames
5697db17  refactor(arduino): validate EEPROM metadata without full stack copy
e8142be7  fix(arduino): define EEPROM metadata constants
dc5f5606  fix(arduino): match proven keypad scan direction
cc18247c  test(arduino): protect compatible keypad scan direction
```

Не объявлять commits после последнего operator-confirmed build GREEN без фактического Actions/PlatformIO результата.

## Принятое целевое разделение

### Arduino Uno — оставить

- физический START;
- единоличное управление SSR;
- Hall sensor на `A0`;
- простой realtime threshold/hysteresis и подсчёт витков;
- winding state machine;
- LCD 1602;
- проверенная библиотека Keypad;
- buzzer;
- минимальный CMP1 UART: принять JOB/control, отправить `RUN_STARTED/RUN_COMPLETED`, ACK/retry;
- EEPROM pending completed events и exact identifiers.

### ESP32 — перенести

- расширенный тест датчика Hall;
- поток/агрегацию диагностической телеметрии;
- расчёт рекомендуемых threshold/hysteresis/direction;
- UI калибровки и отображение результатов;
- хранение расширенных результатов теста;
- orchestration явной операторской калибровки.

ESP32 не управляет SSR и не создаёт автоматический START. Hall остаётся физически подключён к Arduino `A0`; ESP32 получает только bounded diagnostic samples/results по UART.

## Согласованный автономный Hall calibration handshake

```text
ESP32 CAL_ARM
  -> Arduino показывает запрос на LCD
  -> оператор подтверждает на Arduino
  -> только отдельный физический START разрешает вращение
  -> Arduino собирает bounded raw Hall samples/statistics
  -> Arduino передаёт данные ESP32
  -> ESP32 рассчитывает threshold/hysteresis/direction
  -> ESP32 возвращает CAL_PROPOSAL + source sample identity/CRC
  -> Arduino проверяет диапазоны, identity, safe idle/calibration state
  -> Arduino показывает результат оператору
  -> оператор подтверждает применение на Arduino
  -> Arduino атомарно сохраняет профиль в EEPROM + CRC/version
  -> Arduino отвечает CAL_APPLIED с фактически сохранённым профилем
  -> ESP32 сохраняет зеркальную копию для UI/audit
```

Правила:

- команда ESP32 только arm/request; она никогда не запускает двигатель;
- локальное подтверждение и физический START принадлежат Arduino;
- SSR включается только Arduino и только внутри явно подтверждённой calibration state;
- потеря UART/ESP32 немедленно завершает/abort calibration fail-safe с SSR OFF;
- proposal ESP32 не применяется автоматически;
- Arduino независимо валидирует допустимые threshold/hysteresis/direction;
- authoritative runtime profile хранится в Arduino EEPROM;
- ESP32 хранит mirror/audit copy, но её отсутствие не мешает обычной работе;
- после reboot Arduino загружает последний CRC-valid applied profile и не продолжает calibration;
- несовпадение mirror не перезаписывает Arduino автоматически;
- production Hall counting полностью автономен после сохранения, ESP32 для намотки не требуется.

## План реализации

### Этап 1 — вернуть проверенную клавиатуру

1. Fetch актуальные `firmware/arduino/src/main.cpp`, `platformio.ini` и blob SHA.
2. Вернуть `chris--a/Keypad @ ^3.1.1`, прежние `KeyMap/RowPins/ColPins/Keypad`.
3. Удалить compact scanner и его тестовые assertions.
4. Не менять раскладку, pins 2..9, emergency `D * # D` и input semantics.
5. Собрать `pio run -e uno`; записать RAM/Flash.
6. Physical smoke: `1`, `#`, `*`, `D`; SSR/двигатель обесточены.

### Этап 2 — минимизировать Hall runtime на Arduino

1. Сохранить `CM_HallTurnSource`, threshold/hysteresis, direction и turn counting.
2. Удалить из Arduino runtime ownership расширенные `HallCalibrationService` и `HallTelemetryService`.
3. Удалить неиспользуемые calibration/telemetry state, commands и 176-byte formatting paths из Arduino transport.
4. Не изменять motor permit/SSR fail-safe: вне нормальной winding state SSR OFF.
5. Сохранить settings persistence только в минимальной форме, необходимой realtime Hall detector.
6. Собрать и измерить; целевой static headroom минимум 350–400 байт.

### Этап 3 — перенести Hall test/analysis на ESP32

1. Сначала проверить существующие ESP32 `CM_HardwareControlWeb`/UART receiver owners и не создавать дубликаты.
2. Добавить bounded diagnostic sampling protocol без SSR/START commands.
3. ESP32 рассчитывает min/max/baseline/recommended threshold/hysteresis/direction.
4. Apply settings только явным действием оператора и только когда Arduino сообщает безопасное idle state.
5. Reboot не продолжает calibration/apply автоматически.
6. Добавить exact protocol/UI/safety regressions.

### Этап 4 — дополнительный запас Uno

1. Убрать оставшиеся 176-byte hardware-control stack frames.
2. Затем заменить тяжёлый LCD/Wire path компактным PCF8574 driver только при наличии compile/physical parity test.
3. Не уменьшать EEPROM pending capacity или exact local program provenance без отдельного доказательства.
4. Повторить ELF map и зафиксировать крупнейшие owners.

### Этап 5 — hardware gate

1. Arduino only: boot/home/keypad/LCD.
2. Hall manual rotation test без SSR.
3. ESP32<->Arduino JOB receive; Arduino остаётся READY.
4. Только физический START создаёт `RUN_STARTED`.
5. Exact `RUN_COMPLETED` доставляется/повторяется до ACK.
6. Reboot не auto-resume.
7. Material writeoff остаётся ручным exact `spool_id + source_session_id + source_run_id`.

## Safety invariants — не ослаблять

- no automatic physical START;
- no automatic START between repeat runs;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout never proves Arduino idle;
- `RUN_COMPLETED` never automatically writes off material;
- writeoff manual, exact run/session/spool;
- cancellation never deletes immutable run/history;
- EEPROM pending events never clear automatically during diagnostics/migration.

## Порядок чтения в новом чате

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/65_ARDUINO_SOURCE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

## Инструкция для следующего чата

Продолжать сразу с этапа 1. Перед каждым update fetch актуальный файл из `cmp-protocol-v1` и использовать current blob SHA. Для новых файлов сначала подтвердить 404. Не использовать `main`. Не очищать EEPROM. Не утверждать CI/build/hardware GREEN без фактического результата. После каждого измеренного блока обновлять этот файл и `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
