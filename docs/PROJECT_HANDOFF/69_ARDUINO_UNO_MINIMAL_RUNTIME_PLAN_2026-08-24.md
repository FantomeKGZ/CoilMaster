# CoilMaster — план минимального runtime Arduino Uno

Дата: **2026-08-24**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Назначение

Этот файл — быстрый handoff для аппаратного восстановления CoilMaster. Текущая цель: оставить Arduino Uno минимальным безопасным realtime-контроллером, сохранить проверенную клавиатуру и перенести расширенный тест/анализ Hall на ESP32 без передачи ESP32 права управлять SSR или автоматически запускать двигатель.

## Статус текущего блока 2026-08-24

- Проверенная библиотека `chris--a/Keypad @ ^3.1.1` возвращена; compact scanner удалён.
- Первый build после возврата Keypad был **FAILED**: Flash `32466/32256`, RAM `1903/2048`.
- После переноса recommendation math на ESP32 и удаления `HallTelemetryService` из production runtime фактический Actions build commit `0c146e1b` **SUCCESS**:
  - Flash `31390/32256` = 97.3%, запас 866 bytes;
  - RAM `1888/2048` = 92.2%, static headroom 160 bytes.
- Host tests для этого блока подтверждены GREEN, включая Arduino entrypoint, JOB lifecycle, release safety и Hall calibration contracts.
- Текущий image теперь помещается в Uno, но **160 bytes SRAM headroom недостаточно** для финального hardware gate. Целевой static headroom остаётся минимум 350–400 bytes.
- `CM_HallCalibrationAnalyzer` на ESP32 принимает bounded measurement summary и рассчитывает threshold/hysteresis/direction.
- Arduino `CM_HallCalibrationService` больше не рассчитывает recommendation; Uno остаётся measurement/safety owner.
- `CM_HardwareControlWeb` на ESP32 анализирует полученный summary через `HallCalibrationAnalyzer::analyzeSummary()` и публикует recommendation в UI.
- `HallTelemetryService` удалён из production runtime ownership: Arduino отдаёт bounded одиночный Hall snapshot без собственного telemetry window aggregation.
- Начат следующий SRAM optimization batch:
  - `4fac173f` — AVR `TWI_BUFFER_LENGTH` уменьшен до 8; production Uno использует I2C только для PCF8574 LCD path, где записи однобайтовые;
  - `58a98b78` / `81ec5791` / `ccbe08f5` — calibration result больше не дублируется как постоянный статический объект; CMP1 wire shape сохраняется с neutral recommendation placeholders для анализа ESP32.
- Этот новый SRAM batch **ещё не имеет подтверждённых Actions size numbers**; GREEN для него не объявлять до фактического build.
- EEPROM не очищался. Physical START и SSR authority остались на Arduino.

### Политика проверки до завершения оптимизации

По решению пользователя от 2026-08-24 промежуточные hardware smoke-tests откладываются до окончания оптимизации.

До финального optimization checkpoint выполнять только:

```text
code review
host regressions
PlatformIO/Actions compile
Flash/RAM size gates
protocol/safety contracts
```

Не просить промежуточно нажимать keypad, вращать Hall, запускать двигатель или выполнять отдельные Serial smoke-tests. После завершения оптимизации провести один полный hardware acceptance gate.

### Коммиты текущего блока

```text
7340662d  build(arduino): restore Keypad dependency
0f4e2daf  fix(arduino): restore proven Keypad runtime
d80b8971  test(arduino): restore Keypad runtime contract
cb3863c0  feat(esp32): add Hall calibration analyzer
1f6fd7df  feat(esp32): port Hall recommendation analysis
11f7569f  feat(esp32): analyze Hall summary data
eb7750ec  feat(esp32): analyze Hall summary data
3fb83698  refactor(arduino): return Hall measurement summary
b2e3550c  feat(esp32): own Hall calibration recommendation
68f01b94  test(hall): enforce ESP32 recommendation ownership
0c146e1b  refactor(arduino): minimize Hall telemetry runtime
4fac173f  build(arduino): shrink AVR TWI buffers
58a98b78  refactor(arduino): compact Hall calibration state
81ec5791  refactor(arduino): compact Hall calibration state
ccbe08f5  refactor(arduino): compact Hall calibration state
```

## Подтверждённая аппаратная картина

- LCD 1602 физически исправен: I2C scanner стабильно находил `0x27`, отдельный `LiquidCrystal_I2C(0x27,16,2)` test выводил текст.
- Production firmware первоначально показывала блоки, затем циклический `CM BOOT`.
- Изоляция внешних модулей не устранила reset-loop.
- USB diagnostic показал `M=0`; ELF-карта и PlatformIO подтвердили исчерпание SRAM.
- Исходный production baseline: RAM `1965/2048` (95.9%), Flash `31300/32256` (97.0%), только 83 байта static headroom.
- Крупнейшие ELF owners: `espTransport=355`, legacy `Keypad=111`, пять Wire/TWI buffers по 32 байта.
- После compact keypad + streaming winding UART: RAM `1770/2048` (86.4%), Flash `31022/32256` (96.2%), 278 байт static headroom.
- Production запускался и не перезапускался на compact-keypad image, но compact keypad физически не реагировал; поэтому самописный scanner не возвращать.
- USB diagnostic проходил `SERIAL/LCD/UART/EEPROM/HW_SETTINGS/SSR_SAFE_OFF/BUZZER/KEYPAD/START_BUTTON/STATE/OUTPUTS/READY`.
- До удаления библиотеки Keypad клавиатура работала.
- Пользователь подтвердил: до добавления расширенного Hall test/calibration блока система работала нормально.

## Принятое целевое разделение

### Arduino Uno — оставить

- физический START;
- единоличное управление SSR;
- Hall sensor на `A0`;
- простой realtime threshold/hysteresis и подсчёт витков;
- winding state machine;
- LCD 1602;
- проверенную библиотеку Keypad;
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
  -> Arduino локально подтверждает calibration state
  -> только отдельный физический START разрешает вращение
  -> Arduino собирает bounded Hall measurement summary
  -> Arduino передаёт данные ESP32
  -> ESP32 рассчитывает threshold/hysteresis/direction
  -> ESP32 возвращает CAL_PROPOSAL + source sample identity/CRC
  -> Arduino проверяет диапазоны, identity, safe idle/calibration state
  -> оператор подтверждает применение на Arduino
  -> Arduino атомарно сохраняет профиль в EEPROM + CRC/version
  -> Arduino отвечает CAL_APPLIED с фактически сохранённым профилем
  -> ESP32 сохраняет зеркальную копию для UI/audit
```

Правила:

- команда ESP32 только arm/request; она никогда не запускает двигатель;
- локальное подтверждение и физический START принадлежат Arduino;
- SSR включается только Arduino и только внутри явно подтверждённой calibration state;
- потеря UART/ESP32 завершает/abort calibration fail-safe с SSR OFF;
- proposal ESP32 не применяется автоматически;
- Arduino независимо валидирует допустимые threshold/hysteresis/direction;
- authoritative runtime profile хранится в Arduino EEPROM;
- ESP32 хранит mirror/audit copy, но её отсутствие не мешает обычной работе;
- после reboot Arduino загружает последний CRC-valid applied profile и не продолжает calibration;
- несовпадение mirror не перезаписывает Arduino автоматически;
- production Hall counting полностью автономен после сохранения, ESP32 для намотки не требуется.

## План реализации

### Этап 1 — вернуть проверенную клавиатуру

Кодовая часть выполнена:

1. `chris--a/Keypad @ ^3.1.1` возвращён.
2. Прежние `KeyMap/RowPins/ColPins/Keypad` восстановлены.
3. Compact scanner удалён.
4. Раскладка, pins 2..9, emergency `D * # D` и input semantics сохранены.
5. Production image теперь снова собирается после Hall minimization.
6. Physical keypad smoke отложен до финального hardware acceptance после завершения оптимизации.

### Этап 2 — минимизировать Hall runtime на Arduino

1. Сохранить `CM_HallTurnSource`, threshold/hysteresis, direction и turn counting.
2. Удалить расширенную telemetry aggregation из Arduino runtime.
3. Оставить только минимальный calibration sampling/safety owner до завершения handshake migration.
4. Не изменять motor permit/SSR fail-safe.
5. Сохранить settings persistence, необходимую realtime Hall detector.
6. Собрать и измерить; целевой static headroom минимум 350–400 байт.

Статус:

- `HallTelemetryService` уже исключён из production runtime;
- recommendation math уже исключён из Uno;
- calibration static result duplication удаляется текущим SRAM batch;
- TWI low-level buffers сокращены;
- следующий gate — фактические Flash/RAM numbers нового batch.

### Этап 3 — перенести Hall test/analysis на ESP32

1. Bounded diagnostic data без SSR/START commands.
2. ESP32 рассчитывает min/max/baseline/recommended threshold/hysteresis/direction.
3. Apply settings только явным действием оператора и только когда Arduino сообщает безопасное состояние.
4. Reboot не продолжает calibration/apply автоматически.
5. Добавить exact protocol/UI/safety regressions.

Статус: recommendation calculation перенесён. Bounded telemetry передаётся как snapshots; полноценный proposal/apply identity handshake ещё нужно завершить.

### Этап 4 — дополнительный запас Uno

Приоритеты после текущего TWI/calibration batch:

1. Повторить size measurement.
2. Если static headroom <350 bytes, оптимизировать Keypad только без изменения proven 3.1.1 scanning/debounce semantics.
3. Затем уменьшить постоянный UART RX owner только через streaming parser, сохранив максимальный 10-coil JOB и CRC semantics.
4. Не уменьшать EEPROM pending capacity или exact local program provenance.
5. Не менять LCD physical driver до необходимости; текущий первый шаг ограничен штатным `TWI_BUFFER_LENGTH`.

### Этап 5 — единый финальный hardware gate

Промежуточные hardware smoke-tests не выполнять. После завершения оптимизации проверить одним циклом:

1. Arduino boot/home + отсутствие reset-loop.
2. LCD 1602.
3. Keypad: `1`, `#`, `*`, `D`, emergency `D * # D`.
4. Hall manual rotation test без SSR.
5. Hall calibration handshake ESP32<->Arduino.
6. ESP32<->Arduino JOB receive; Arduino остаётся READY до физического START.
7. Только физический START создаёт `RUN_STARTED` и разрешает SSR по Arduino state.
8. Exact `RUN_COMPLETED` доставляется/повторяется до ACK.
9. Reboot не auto-resume и не продолжает calibration.
10. Material writeoff остаётся ручным exact `spool_id + source_session_id + source_run_id`.

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

Продолжать с текущего SRAM optimization batch. Сначала получить фактический Actions/PlatformIO build после `4fac173f` + `58a98b78` + `81ec5791` + `ccbe08f5`, записать Flash/RAM, затем при необходимости оптимизировать Keypad capacity без изменения proven scan semantics и после этого UART RX через streaming parser. Hardware checks не запрашивать до полного завершения оптимизации; затем выполнить единый hardware acceptance gate. Перед каждым update fetch актуальный файл из `cmp-protocol-v1` и использовать current blob SHA. Для новых файлов сначала подтвердить 404. Не использовать `main`. Не очищать EEPROM. Не утверждать CI/build/hardware GREEN без фактического результата.
