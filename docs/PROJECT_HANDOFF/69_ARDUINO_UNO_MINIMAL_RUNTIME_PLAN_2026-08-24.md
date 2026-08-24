# CoilMaster — план минимального runtime Arduino Uno

Дата: **2026-08-24**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Цель

Arduino Uno остаётся минимальным authoritative realtime/safety controller. Всё тяжёлое Hall test/calibration analysis переносится на ESP32.

## Что обязательно остаётся на Uno

- Hall A0 read для обычной намотки;
- минимальный realtime threshold/hysteresis/debounce/direction;
- автономный turn counting;
- physical START;
- SSR ownership/control;
- local keypad confirmation;
- calibration abort/timeouts/fail-closed state;
- exact proposal identity gate;
- CRC/versioned EEPROM Hall profile;
- local `#` перед EEPROM apply.

Нормальная намотка обязана работать без ESP32 после сохранённой калибровки.

## Что перенесено на ESP32

Во время extended Hall test:

```text
Uno A0 -> CAL_SAMPLE raw ADC -> ESP32
ESP32 -> baseline average / min / max / span / samples / duration
      -> analyzer / recommendation / history / UI
```

Wire:

```text
CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC
CMP1|CAL_SAMPLE|RUN|raw|sequence|elapsed_ms|C|CRC
```

Uno использует уже существующее чтение Hall; второго `analogRead()` ради ESP32 нет.

## Safety calibration flow

```text
Web -> ESP32 CAL_ARM
 -> Uno WAITING_LOCAL_CONFIRM
 -> operator local confirmation
 -> ARMED_WAITING_START, SSR OFF, raw baseline stream
 -> separate physical START
 -> RUNNING, Uno-only motor permit/SSR authority
 -> raw RUN stream
 -> ESP32 analyzer/recommendation
 -> exact CAL_PROPOSAL
 -> Uno WAITING_APPLY_CONFIRM, SSR OFF
 -> local #
 -> EEPROM
 -> CAL_APPLIED
```

`CAL_ARM` никогда не запускает двигатель. START до local confirmation не запускает тест. START в `WAITING_APPLY_CONFIRM` не сохраняет settings и не запускает motor.

## Verified size history

До active raw TX, Actions `32734442579` на commit `8dc72bf...`:

```text
RAM   1220 / 2048 = 59.6%
Flash 32084 / 32256 = 99.5%
free Flash 172 B
```

После включения active raw TX, Actions `32734516276` на `094ad9f...`:

```text
RAM   1222 / 2048 = 59.7%
Flash 32382 / 32256 = 100.4%
overflow 126 B
```

Этот overflow является причиной текущего size-recovery migration; safety/runtime compile дошёл до link/size gate.

## Выполненный size-recovery

Commits:

```text
173532480d8f41475acd0070399b7b9e61e00204  result identity-only direction
3272cabbeb7654ac185450f678491a918319d126  remove Uno baseline/min/max aggregation
79bbaf907548f96e241a3c2064786ceb4044132d  enforce ESP32-only aggregation
d527cc6a0877342a7d05622d0cf1391d0e0784a2  identity-only HallCalibrationResult struct
0bd3bb4109671fc8a440460046cbe0c9fa3743dc  remove legacy Uno result fields
8e7b00536b1d105ef50d02e9900b0b0c4414485d  literal legacy CAL_RESULT statistics
69bb08191872111b9801576c1a0a60bd7b9d8528  regression for identity-only result
```

Удалено с Uno:

```text
m_baselineSum
m_minAdc
m_maxAdc
m_resultDurationMs
baseline average calculation
run min/max calculation
summary-dependent measurement identity
legacy HallCalibrationResult measurement fields
```

Оставлено:

```text
m_baselineSamples   // physical START safety gate
m_runSamples        // non-empty run + raw sequence
m_measurementId     // exact transient proposal identity
```

`HallCalibrationResult` теперь содержит только `uint32_t measurementId`.

Для совместимости ESP32 parser Uno всё ещё отправляет legacy shape, но statistics — literal zeroes:

```text
CMP1|CAL_RESULT|INVALID|0|0|0|0|0|RISING|0|0|measurement_id|C|CRC
```

ESP32 до analyzer заменяет эти поля собственным raw-stream summary.

## Текущий verification status

Последний полностью измеренный active-raw Uno image `094ad9f...` был size RED из-за `32382 > 32256`.

Новый size-recovery HEAD после `69bb0819...`/`6f710e97...` **ещё нельзя называть GREEN и нельзя приписывать ему новый Flash/RAM размер без Actions evidence**.

Host runs перед этим:

```text
32734442578 SUCCESS
32734516435 SUCCESS
32734591226 SUCCESS
32734680453 RED только из-за brittle documentation run-id audit
```

Brittle audit исправлен в `79bbaf907...`; runtime failure там не было.

## Safety invariants — не ослаблять

- no automatic physical START;
- no automatic START between repeat runs;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- proposal never auto-applies;
- EEPROM apply требует local `#`;
- lost UART during calibration/apply => abort/fail closed/SSR OFF;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically writes off material;
- writeoff remains manual exact session + run + immutable spool;
- cancellation never deletes immutable history.

## Следующий active block

1. Получить новый Uno Actions size на `6f710e97...` или descendant.
2. Сравнить с active-raw overflow `32382 B` и pre-raw `32084 B`.
3. Если Flash headroom всё ещё мал — отдельным experiment упростить `CAL_SAMPLE` formatter/temporary raw bridge; не смешивать несколько size changes.
4. После достаточного Uno headroom закрыть ESP32 semantic hardening: `CAL_APPLIED` mirror должен range-validate threshold 1..1023, hysteresis 1..512 и `< threshold`, debounce 1..1000 перед authoritative mirror update.
5. Найти существующего DS3231 owner перед добавлением human timestamp в Hall history; не создавать второго RTC owner.
6. Синхронизировать `06_ACTIVE_WORK_AND_NEXT_STEPS.md` после software gate stabilization.
7. Только после завершения оптимизации — единый hardware acceptance.

## Hardware policy

По решению пользователя промежуточные hardware smoke-tests не выполнять. До окончания оптимизации использовать только code review, CI/compile, memory и protocol/safety contracts.

## Финальный hardware acceptance

После software stabilization одним циклом:

1. boot/home без reset loop;
2. LCD 1602;
3. keypad `1`, `#`, `*`, `D`, emergency `D * # D`;
4. Hall manual rotation без SSR;
5. `CAL_ARM -> local confirm -> baseline -> physical START`;
6. UART disconnect during calibration -> ABORTED, SSR OFF;
7. raw result -> ESP32 recommendation -> exact proposal -> WAITING_APPLY_CONFIRM -> local `#` -> EEPROM;
8. START в WAITING_APPLY_CONFIRM не сохраняет и не запускает;
9. reboot: no calibration/proposal resume, load last CRC-valid Hall profile;
10. remote JOB stays READY until physical START;
11. only physical START produces RUN_STARTED and permits SSR;
12. RUN_COMPLETED retries until ACK;
13. material writeoff manual exact spool/session/run.

## Новый чат — порядок чтения

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/71_HALL_RAW_STREAM_MIGRATION_2026-08-24.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/70_HALL_CALIBRATION_HISTORY_2026-08-24.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
```

Если `06` противоречит `71`/этому файлу по Hall migration, `71` и этот файл новее.

## Инструкция продолжения

Работать только из `cmp-protocol-v1`. Перед каждым изменением существующего файла fetch актуальный blob SHA. Не использовать `main`. Не запрашивать hardware test до окончания оптимизации. Первое действие — проверить новый Uno/host Actions на текущем HEAD и снять точный Flash/RAM; затем продолжать только измеримыми size experiments.
