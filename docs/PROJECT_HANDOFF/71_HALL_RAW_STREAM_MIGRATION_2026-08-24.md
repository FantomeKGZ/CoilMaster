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

## Что перенесено на ESP32

ESP32 является owner для extended calibration measurement processing:

- baseline accumulation/average;
- run min/max/span;
- sample accounting for analysis;
- recommendation threshold/hysteresis/direction;
- Web/UI graphs/status;
- last-10 calibration history on SD;
- proposal staging/reconciliation;
- authoritative EEPROM mirror after `CAL_APPLIED`/`CFG_GET`.

## Active raw path

```text
Uno A0 -> CAL_SAMPLE -> ESP32 UartEventReceiver -> HallCalibrationRawCollector
        -> existing CAL_RESULT correlation -> HallCalibrationAnalyzer
```

Во время `ARMED_WAITING_START` Uno отправляет уже прочитанный ADC:

```text
CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC
```

Во время `RUNNING`:

```text
CMP1|CAL_SAMPLE|RUN|raw|sequence|elapsed_ms|C|CRC
```

Второго `analogRead()` не добавлено.

ESP32 receiver валидирует sample, собирает baseline/min/max/sample count/duration и при получении legacy `CAL_RESULT` сохраняет Uno-owned `measurement_id`, но подменяет измерительные поля ESP32-owned raw summary.

## Uno size-recovery migration

Первый реально включённый raw TX образ на commit:

```text
094ad9f1f28d7e10504ee48036f5398a52dde3cf
```

не помещался:

```text
Actions 32734516276
RAM   1222 / 2048 = 59.7%
Flash 32382 / 32256 = 100.4%
overflow 126 B
```

Это подтвердило, что нельзя держать одновременно raw TX и старую Uno extended aggregation.

Выполненный size-recovery block:

```text
173532480d8f41475acd0070399b7b9e61e00204  refactor(hall): make Uno calibration result identity-only
3272cabbeb7654ac185450f678491a918319d126  refactor(hall): remove Uno calibration aggregation
79bbaf907548f96e241a3c2064786ceb4044132d  test(hall): enforce ESP32-only calibration aggregation
d527cc6a0877342a7d05622d0cf1391d0e0784a2  refactor(hall): make Uno result identity-only struct
0bd3bb4109671fc8a440460046cbe0c9fa3743dc  refactor(hall): drop legacy Uno result fields
8e7b00536b1d105ef50d02e9900b0b0c4414485d  refactor(hall): emit literal legacy result fields
69bb08191872111b9801576c1a0a60bd7b9d8528  test(hall): enforce identity-only Uno result
a8768eb37f95935c447831fc906d237644833c0a  test(hall): align safety audit with raw identity flow
```

С Uno удалены:

```text
m_baselineSum
m_minAdc
m_maxAdc
m_resultDurationMs
baseline average calculation
run min/max calculation
summary-dependent measurement identity
```

`HallCalibrationResult` теперь содержит **только**:

```text
uint32_t measurementId
```

На Uno calibration runtime оставлены только:

```text
m_baselineSamples   // local physical-START safety gate
m_runSamples        // reject empty measurement / raw sequence
m_measurementId     // exact transient proposal correlation
```

Legacy wire shape `CAL_RESULT` пока сохранён без изменения parser contract, но measurement fields печатаются литералами:

```text
CMP1|CAL_RESULT|INVALID|0|0|0|0|0|RISING|0|0|measurement_id|C|CRC
```

Это намеренный compatibility carrier. ESP32 заменяет zero statistics собственным valid raw summary до `HallCalibrationAnalyzer`.

`measurement_id` формируется только из Uno-local transient timing/counter state и сохраняется при `finish()`. Он не зависит от ESP32 baseline/min/max и продолжает exact-ID gate для `CAL_PROPOSAL`.

## Verified size checkpoint после raw migration

Actions run:

```text
32736552491  build-uno SUCCESS
```

Exact checkout:

```text
8e7b00536b1d105ef50d02e9900b0b0c4414485d
```

Memory:

```text
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31738 / 32256 = 98.4% free 518 B
```

Сравнение:

```text
до active raw TX:       Flash 32084, free 172 B
первый active raw TX:   Flash 32382, overflow 126 B
после ESP32 migration:  Flash 31738, free 518 B
```

Итого size-recovery освободил **644 B Flash** относительно переполненного raw-TX образа и дал **+346 B headroom** относительно последнего рабочего pre-raw образа 32084 B.

RAM также улучшилась до 1213 B used / 835 B free.

## Verified host checkpoint после raw migration

После синхронизации старого Hall safety audit:

```text
32739185113  host-tests SUCCESS
32739260013  host-tests SUCCESS
32740958702  host-tests SUCCESS
32741039242  host-tests SUCCESS
32744028372  host-tests SUCCESS
32744056448  host-tests SUCCESS
```

Последние run полностью GREEN, включая:

```text
Hall calibration safety contracts
Hall lost apply reconciliation
Uno Hall parser ownership
Hall raw migration ownership
Hall history and SD reference contracts
```

То есть software contracts схемы `Uno raw + safety / ESP32 analysis` подтверждены.

## Отклонённый Flash experiment — manual CAL_SAMPLE formatter

Experiment:

```text
219ff5b56db381742ce71d5faeb4f96b11a78f55  perf(uno): format Hall raw samples without snprintf
e1cd19756b4fac1e4718a5b6e501fc21f0dc3b54  test(hall): cover compact raw sample formatter
```

Actions:

```text
32740897611  build-uno SUCCESS
checkout 219ff5b56db381742ce71d5faeb4f96b11a78f55
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 32192 / 32256 = 99.8% free 64 B
```

То есть manual bounded append + `ultoa` **ухудшил Flash на 454 B** относительно verified checkpoint `31738 B`. RAM не изменился.

Experiment признан невыгодным и откатан:

```text
faaa8c4f47ccc7988ba665f1572c972d1127a4ed  revert(hall): restore compact snprintf raw formatter
7d3c18af057259ec5a148738694eac452a14cfdd  test(hall): restore snprintf raw formatter contract
```

Active implementation снова использует старый `snprintf_P("CMP1|CAL_SAMPLE|%S|%u|%u|%lu|C", ...)`, который на AVR/LTO оказался заметно компактнее ручной сборки.

## Bridge micro-experiment

Commits:

```text
9cd7be9ee17e3f62ae8de59b3c0ecf2bfe63e7b1  perf(uno): inline Hall raw bridge registration
e6325c86fbd84445df3c96564cd601b3f0df6baa  fix(uno): remove duplicate bridge registration definition
```

Первый commit не компилировался из-за временного duplicate constructor definition. Второй исправил это.

Verified Actions:

```text
32744056466  build-uno SUCCESS
checkout e6325c86fbd84445df3c96564cd601b3f0df6baa
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31738 / 32256 = 98.4% free 518 B
```

Дельта против baseline: **0 B Flash / 0 B RAM**. Поэтому сам registration thunk больше не является перспективным объектом оптимизации.

## Active size experiment — compact transient measurement token

Commit:

```text
02d9cd7e3c0679ae77d645a550af4f933b355e76  perf(uno): simplify transient Hall measurement token
```

Из `measurementIdentity()` убраны три xorshift-прохода. Новый token остаётся Uno-owned и строится только из transient phase timestamps + run sample count. Ноль по-прежнему зарезервирован как `no measurement`.

Это **не security token** и никогда не персистится; его единственная роль — exact correlation между Uno `CAL_RESULT` и возвращённым ESP32 `CAL_PROPOSAL`. Exact equality gate в `beginApplyConfirm()` не менялся.

Experiment пока **не имеет verified Uno size**. Следующий `build-uno` на `02d9cd7e...` или descendant должен сравниваться с baseline `31738 Flash / 1213 RAM`.

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

## Safety invariants

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

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/71_HALL_RAW_STREAM_MIGRATION_2026-08-24.md
docs/PROJECT_HANDOFF/70_HALL_CALIBRATION_HISTORY_2026-08-24.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
```

Если старые `69`/`06` противоречат этому документу по Hall migration, этот документ новее и имеет приоритет до синхронизации.

## Инструкция следующему чату

Продолжать только `cmp-protocol-v1`. Перед каждым update fetch current blob SHA. Не использовать `main` как source. Не просить hardware smoke-test до конца оптимизации.

Первое действие: проверить host + Uno Actions на `02d9cd7e...` или descendant. Verified baseline для сравнения: `31738 Flash / 1213 RAM`. Если compact transient token уменьшил Flash и CI GREEN — оставить. Если дельта нулевая/хуже или audit/build ломается — откатить только этот experiment. Manual formatter `219ff5b...` не повторять; bridge registration micro-experiment `e6325c86...` дал 0 B.
