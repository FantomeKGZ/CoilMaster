# CoilMaster — Hall raw stream migration

Дата: **2026-08-24**  
Source of truth: **`cmp-protocol-v1`** only.

## Решение пользователя

Расширенный тест и калибровка Hall переносятся с Arduino Uno на ESP32 настолько, насколько это возможно без ослабления realtime/safety.

Целевая схема:

```text
WEB -> ESP32 -> CAL ARM -> Arduino
Arduino -> local operator confirmation -> physical test start
Arduino A0 -> bounded raw ADC stream -> ESP32
ESP32 -> baseline/min/max/span/analyzer/recommendation/history/UI
ESP32 -> exact CAL_PROPOSAL -> Arduino
Arduino -> local confirmation -> EEPROM
```

## Что остаётся authoritative на Arduino Uno

Никогда не переносить на ESP32:

- физическое чтение Hall A0 для обычной намотки;
- минимальный realtime threshold/hysteresis/debounce/direction;
- автономный turn counting во время намотки;
- physical START;
- SSR ownership/control;
- calibration safety state: local confirmation, physical-start gate, abort/timeouts;
- final Hall settings + CRC/versioned EEPROM;
- exact proposal identity validation;
- local confirmation перед EEPROM apply.

Обычная намотка обязана работать после сохранённой калибровки даже при отсутствующем/зависшем ESP32.

## Что переносится на ESP32

ESP32 является owner для extended calibration measurement processing:

- baseline accumulation/average;
- run min/max/span;
- sample accounting for analysis;
- recommendation threshold/hysteresis/direction;
- Web/UI graphs/status;
- last-10 calibration history on SD;
- proposal staging/reconciliation;
- authoritative EEPROM mirror after `CAL_APPLIED`/`CFG_GET`.

## Первый реализованный migration layer

Добавлены:

```text
firmware/esp32/src/CM_HallCalibrationRawCollector.h
firmware/esp32/src/CM_HallCalibrationRawCollector.cpp
```

Commits:

```text
5b827d39c95062b988e30bd0457dbc77e39fac33  feat(hall): add ESP32 raw calibration collector
0ffcc843e6bcf72afb8c1fe24423e8961b949b95  feat(hall): implement ESP32 raw calibration collector
```

`HallCalibrationRawCollector` уже умеет на ESP32:

- принимать bounded raw ADC 0..1023;
- накапливать baseline;
- вычислять baseline average;
- принимать run samples;
- вычислять min/max;
- считать samples/duration;
- выдавать validated summary для существующего `HallCalibrationAnalyzer`.

Этот слой пока подготовительный: wire raw sample ещё не подключён, поэтому старый Uno summary нельзя удалять до следующего шага.

## Следующий wire contract

Предпочтительное направление — отдельный компактный calibration-only sample frame, а не тяжёлый normal telemetry frame.

Рекомендуемый bounded wire shape:

```text
CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC
CMP1|CAL_SAMPLE|RUN|raw|sequence|elapsed_ms|C|CRC
```

Требования:

- `raw`: 0..1023;
- `sequence`: transient monotonic counter, допускается rollover;
- `elapsed_ms`: относительно текущего calibration phase;
- sample stream не имеет права START/SSR semantics;
- CRC обязателен;
- invalid/oversized sample ignored;
- UART loss => Arduino abort + SSR OFF по существующему peer timeout;
- sample stream не записывается в EEPROM.

Финал измерения должен содержать только correlation identity/phase completion, а summary должен собираться ESP32. Точный финальный frame закрепить regression-тестом до удаления старого `CAL_RESULT` summary.

## Порядок миграции — не менять

1. ESP32 raw collector — **сделано**.
2. Добавить compact `CAL_SAMPLE` TX на Uno и parser/capture на ESP32.
3. Во время `ARMED_WAITING_START` отправлять baseline raw samples; SSR всегда OFF.
4. Только отдельный physical START переводит Arduino в `RUNNING` и разрешает SSR.
5. Во время `RUNNING` Uno отправляет raw samples, ESP32 собирает summary.
6. ESP32 формирует existing analyzer recommendation.
7. Перевести measurement identity на compact Uno-owned transient token, не зависящий от min/max/baseline math.
8. Только после подтверждённого ESP32 raw path удалить с Uno:
   - baseline sum/average computation;
   - run min/max computation;
   - summary result formatter fields, которые больше не нужны;
   - obsolete calibration measurement helpers.
9. Снять отдельный Uno Flash/RAM gate.
10. Обновить Hall audits и handoff.

Не выполнять шаг 8 раньше шага 5/6: нельзя одновременно потерять measurement source и summary source.

## Текущий verified Uno checkpoint до raw migration

Actions run:

```text
32725501435  build-uno SUCCESS
```

Checkout:

```text
8fc226de4e2a3b07a9e75df302f27d69cb17bd7a
```

Memory:

```text
RAM   1219 / 2048 = 59.5%   free 829 B
Flash 32084 / 32256 = 99.5% free 172 B
```

Не переносить эти цифры на новый raw-migration HEAD без нового Actions build.

## Safety invariants

Не ослаблять ни при каких оптимизациях:

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost UART/peer timeout during calibration => abort/fail closed/SSR OFF;
- START while waiting apply confirmation does not save or start;
- proposal never auto-applies;
- local confirmation required before EEPROM write;
- lost `CAL_APPLIED` never triggers proposal replay;
- current EEPROM profile reconciled by `CFG_GET`, but equality does not prove exact apply event;
- normal winding realtime turn count remains Uno-local;
- no automatic material writeoff.

## Hardware policy

По решению пользователя промежуточные hardware tests не запрашивать. Использовать software gates/CI/size/contracts до окончания оптимизации. Затем один полный hardware acceptance.

## Что читать в новом чате

В таком порядке:

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/71_HALL_RAW_STREAM_MIGRATION_2026-08-24.md
docs/PROJECT_HANDOFF/70_HALL_CALIBRATION_HISTORY_2026-08-24.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
```

Если старые `69`/`06` противоречат этому документу по Hall migration, **этот документ новее и имеет приоритет** до их синхронизации.

## Инструкция следующему чату

Продолжать только `cmp-protocol-v1`. Перед каждым update fetch current blob SHA. Не использовать `main` как source. Не просить hardware smoke-test до конца оптимизации. Следующий кодовый шаг: compact `CAL_SAMPLE` wire + ESP32 parser/capture, затем подключить `HallCalibrationRawCollector`; только после GREEN raw-path удалить старое Uno summary computation и измерить Flash savings.
