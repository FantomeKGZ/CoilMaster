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

## Реализованный active raw path

Добавлены ESP32 collector/parser, Uno formatter, временный raw bridge и active receiver aggregation:

```text
firmware/esp32/src/CM_HallCalibrationRawCollector.h
firmware/esp32/src/CM_HallCalibrationRawCollector.cpp
firmware/esp32/src/CM_HallCalibrationRawProtocol.h
firmware/esp32/src/CM_HallCalibrationRawProtocol.cpp
firmware/esp32/src/CM_UartEventReceiver.h
firmware/esp32/src/CM_UartEventReceiver.cpp
Arduino/CM_HallCalibrationProtocol.h
Arduino/CM_HallCalibrationProtocol.cpp
Arduino/CM_HallCalibrationRawBridge.h
Arduino/CM_HallCalibrationRawBridge.cpp
Arduino/CM_UartEventTransport.h
Arduino/CM_HallCalibrationService.cpp
```

Ключевые commits:

```text
5b827d39c95062b988e30bd0457dbc77e39fac33  feat(hall): add ESP32 raw calibration collector
0ffcc843e6bcf72afb8c1fe24423e8961b949b95  feat(hall): implement ESP32 raw calibration collector
2e5887acbfb60e20a2245345957670dda05e2601  feat(hall): add ESP32 raw calibration protocol parser
89797cf11c09b1de2f078d5f0497409ba9b8fb8d  feat(hall): parse bounded raw calibration samples
c4961966f62ffb7506f785b1866d1b768bb2f8b3  feat(hall): define compact raw calibration sample frame
b8b1a7a1811da4ad76329a06d7867ecb4818695e  feat(hall): format compact raw calibration samples
60be6567f77d063c1ce003071fb001ebb37d42dc  test(hall): fix raw parser token audit
07d172b9f1a08c19b4fd714c87f80428e61229fd  feat(hall): collect raw calibration samples in receiver
c0eea2baa08f25e47dd6625b8d1738ad1713c9cf  feat(hall): feed raw calibration samples into ESP32 summary
a0710f4b23b702ea2bc7a1c402ff3f4d9362acd6  feat(hall): add temporary Uno raw sample bridge
0455ab5137b93d7d030b1c26f73a3f552bfcc6b4  feat(hall): bind raw calibration stream to Uno UART
8dc72bf34fe7926c2a1bbd36cb172effee9c2091  feat(hall): publish raw calibration samples through Uno UART
094ad9f1f28d7e10504ee48036f5398a52dde3cf  feat(hall): stream existing calibration ADC samples to ESP32
9639f1b21d5dfbc09a5a104f69ee54c6a50a8942  test(hall): cover active raw calibration stream
```

### Active runtime behavior

Во время `ARMED_WAITING_START` Uno уже делает существующее чтение Hall и из **того же raw значения** отправляет:

```text
CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC
```

Во время `RUNNING` Uno из существующего чтения отправляет:

```text
CMP1|CAL_SAMPLE|RUN|raw|sequence|elapsed_ms|C|CRC
```

Дополнительного `analogRead()` ради ESP32 не добавлено.

`UartEventReceiver` на ESP32:

1. перехватывает `CAL_SAMPLE` до обычного event parser;
2. валидирует frame через `HallCalibrationRawProtocol`;
3. baseline samples складывает в `HallCalibrationRawCollector`;
4. RUN samples складывает туда же и запоминает duration;
5. при legacy `CAL_RESULT` сохраняет Uno-owned `measurement_id`, но при валидном raw summary заменяет измерительные поля `baseline/min/max/sampleCount/duration` на ESP32-owned summary;
6. существующий `HardwareControlWeb -> HallCalibrationAnalyzer` получает уже ESP32-owned measurement summary.

То есть active data path теперь:

```text
Uno A0 -> CAL_SAMPLE -> ESP32 receiver -> RawCollector -> existing CAL_RESULT correlation -> Analyzer
```

Legacy Uno summary пока оставлен как fallback/correlation carrier. Это временно и необходимо до GREEN software gate нового active path.

## Временный Uno raw bridge

`CM_HallCalibrationRawBridge` существует только на период миграции, чтобы не переписывать огромный `firmware/arduino/src/main.cpp` и не смешивать raw TX с actuator logic.

Bridge может только вызвать:

```text
UartEventTransport::sendHallCalibrationSample(...)
```

У него нет START, SSR, EEPROM или machine-state authority.

После удаления старой Uno measurement aggregation bridge следует пересмотреть/упростить; не превращать его в новый general-purpose control owner.

## Wire contract

```text
CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC
CMP1|CAL_SAMPLE|RUN|raw|sequence|elapsed_ms|C|CRC
```

Требования:

- `raw`: 0..1023;
- `sequence`: transient 0..65535; rollover допустим;
- `elapsed_ms`: относительно calibration phase;
- CRC обязателен;
- malformed/oversized samples ignored;
- sample frame не имеет START/SSR semantics;
- sample stream не пишется в EEPROM;
- UART loss по-прежнему обрабатывается Uno peer timeout: abort/fail-closed/SSR OFF.

## Следующий migration block — главный для экономии Uno Flash

После GREEN active raw path удалить с Uno extended measurement ownership.

Оставить на Uno только минимальные counters/tokens, необходимые safety/correlation.

Кандидаты на удаление из `CM_HallCalibrationService`:

```text
m_baselineSum
m_minAdc
m_maxAdc
baseline average calculation
run min/max calculation
summary-dependent measurementIdentity mixing
часть HallCalibrationResult measurement fields на стороне формирования
```

`m_baselineSamples` можно оставить как маленький local gate, чтобы physical START не разрешался до минимального числа baseline reads. Это safety gate, а не extended analysis.

`m_runSamples`/duration можно оставить только в минимальном объёме, если они нужны для completion/correlation.

Новый `measurement_id` должен быть Uno-owned transient token, но **не зависеть от ESP32-computed min/max/baseline**. Proposal по-прежнему обязан совпадать с exact current token.

После удаления старого summary:

1. ESP32 raw collector становится единственным owner baseline/min/max/span;
2. Uno передаёт correlation/completion + raw samples;
3. `CAL_PROPOSAL` остаётся exact-ID;
4. local `#` перед EEPROM apply сохраняется;
5. снять отдельный Uno Flash/RAM Actions gate и сравнить с verified baseline.

## Последний verified Uno checkpoint ДО active raw TX

Actions run:

```text
32732386298  build-uno SUCCESS
32732349464  build-uno SUCCESS
```

Один из проверенных checkout был `b8b1a7a1811da4ad76329a06d7867ecb4818695e`.

Memory:

```text
RAM   1219 / 2048 = 59.5%   free 829 B
Flash 32084 / 32256 = 99.5% free 172 B
```

Эти цифры **не являются** размером active raw-stream HEAD `9639f1b...` и его descendants. Нужен новый successful Uno Actions build.

Host RED runs до commit `60be6567...` были ложным regression failure: audit искал literal `CMP1|CAL_SAMPLE` в token parser. Runtime parser был корректен; audit исправлен на проверки `CMP1` + `CAL_SAMPLE` по отдельным tokens.

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

Продолжать только `cmp-protocol-v1`. Перед каждым update fetch current blob SHA. Не использовать `main` как source. Не просить hardware smoke-test до конца оптимизации.

Текущий active raw path уже подключён. Сначала проверить host + Uno + ESP32 Actions для `9639f1b...`/descendant. Если GREEN и Uno помещается — следующий кодовый шаг: удалить с Uno baseline sum/min/max aggregation и перевести measurement identity на compact Uno-only transient token. Если Uno overflow — сделать это удаление сразу как size-recovery block, не добавляя новую функциональность.
