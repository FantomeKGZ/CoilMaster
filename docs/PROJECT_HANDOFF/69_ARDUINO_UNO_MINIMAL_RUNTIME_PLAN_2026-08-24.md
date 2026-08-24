# CoilMaster — план минимального runtime Arduino Uno

Дата: **2026-08-24**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Назначение

Этот файл — быстрый handoff для аппаратного восстановления CoilMaster. Цель: оставить Arduino Uno минимальным безопасным realtime-контроллером, сохранить проверенную Keypad 3.1.1 electrical/debounce semantics и перенести расширенный Hall test/analysis на ESP32 без передачи ESP32 права управлять SSR или автоматически запускать двигатель.

## Текущий verified baseline — 2026-08-24

Последний полностью подтверждённый software/memory baseline — commit:

```text
10e11bba081d06eb3b149eb9c2bd7b593faa8895
```

GitHub Actions evidence:

```text
32698376710  Arduino Uno build  SUCCESS
32698376682  host-tests         SUCCESS
```

Фактический Uno size:

```text
RAM   1579 / 2048 = 77.1%
free SRAM = 469 bytes

Flash 29660 / 32256 = 92.0%
free Flash = 2596 bytes
```

Целевой static SRAM headroom **350–400 bytes достигнут и превышен**. Агрессивное дальнейшее урезание production Uno только ради памяти сейчас не требуется.

В verified image уже входят:

- bounded local Keypad owner, сохраняющий proven Keypad 3.1.1 scan/debounce semantics, но с `LIST_MAX=1` и `MAPSIZE=4`;
- bufferless PCF8574 LCD transport через AVR hardware TWI без постоянных Wire RX/TX/TWI buffers;
- measurement-only Hall calibration result на Uno;
- bounded одиночная Hall telemetry sample без Uno-side aggregation window;
- ESP32-owned Hall recommendation analysis;
- CMP1 receive buffer 112 bytes, достаточный для worst-case допустимого 10-coil JOB;
- EEPROM pending completed events и exact identifiers/provenance без очистки.

## Текущий Hall safety batch — implementation есть, Actions ещё не подтверждены

После verified `10e11bba` реализован следующий safety block. **Не считать его GREEN, пока новый Actions build/host-tests не подтверждены.**

### Локальное подтверждение на Arduino

Новый обязательный порядок:

```text
ESP32 CAL_ARM
  -> Arduino WAITING_LOCAL_CONFIRM
  -> оператор нажимает # на Arduino
  -> Arduino ARMED_WAITING_START
  -> Arduino собирает baseline
  -> отдельная физическая START
  -> Arduino RUNNING + motorPermit
```

До локального `#` физическая START не запускает calibration и не проходит в обычный winding input.

Любая другая клавиша вместо `#` в `WAITING_LOCAL_CONFIRM` abort-ит calibration. После локального подтверждения любое неожиданное keypad input во время активной calibration по-прежнему abort-ит процедуру.

### UART-loss fail-safe

ESP32 runtime, а не web page, поддерживает calibration link:

```text
CalibrationKeepAliveMs = 1000 ms
CMP1|CAL|GET|C|CRC
```

Arduino обновляет peer-contact timestamp только при валидном parsed CAL traffic. Для активной calibration:

```text
PeerTimeoutMs = 3000 ms
```

Если CAL traffic пропадает, Arduino переводит calibration в `Aborted`; production loop затем принудительно держит SSR OFF. `motorPermit()` остаётся true только в `Running`.

### Коммиты нового safety block

```text
cf5bfad1  feat(hall): require local calibration confirmation
279bff89  feat(hall): require local calibration confirmation
bc66dc49  feat(hall): expose local confirmation state
f0d24d9f  feat(hall): abort calibration on UART loss
e95567ce  feat(hall): abort calibration on UART loss
89134ba5  feat(hall): enforce local calibration confirmation
93c63ce8  feat(hall): keep calibration link alive from ESP32
73acb54e  feat(hall): keep calibration link alive from ESP32
db043264  feat(hall): expose local confirmation in web state
f1c01c8c  feat(hall): show local confirmation step in UI
f2b8d77e  test(hall): enforce local confirmation and UART fail-safe
```

Текущий latest implementation/test commit этого блока:

```text
f2b8d77e73881a0ec836bda6348dfc963dcb863b
```

Для него пока не объявлять build/host GREEN и не переносить verified SRAM цифру с `10e11bba` как будто она измерена после safety changes. Uno получил новый peer-contact timestamp, поэтому actual size нужно снять новым Actions build.

## Политика проверки до завершения оптимизации

По решению пользователя от 2026-08-24 промежуточные hardware smoke-tests откладываются до окончания software optimization/migration.

До финального checkpoint выполнять:

```text
code review
host regressions
PlatformIO/Actions compile
Flash/RAM size gates
protocol/safety contracts
```

Не просить промежуточно нажимать keypad, вращать Hall, запускать двигатель или выполнять отдельные Serial smoke-tests. После завершения software блока провести один полный hardware acceptance gate.

## Подтверждённая аппаратная история

- LCD 1602 физически исправен: I2C scanner находил `0x27`, отдельный LCD test выводил текст.
- Старый production показывал блоки, затем циклический `CM BOOT`.
- USB diagnostic показал `M=0`; ELF/PlatformIO подтвердили SRAM exhaustion.
- Исходный production baseline: RAM `1965/2048`, Flash `31300/32256`, всего 83 bytes static SRAM headroom.
- После раннего compact keypad + streaming UART image: RAM `1770/2048`, но compact keypad физически не реагировал. Этот самописный scanner не возвращать.
- До expanded Hall test/calibration пользователь подтверждал нормальную работу системы.
- После возврата полной Keypad library и Hall minimization `0c146e1b`: RAM `1888/2048`, только 160 bytes free.
- После первого TWI/Hall compaction batch: RAM `1810/2048`, 238 bytes free.
- После bounded Keypad + bufferless LCD + corrected JOB RX: verified `10e11bba` = **469 bytes free SRAM** и **2596 bytes free Flash**.

## Целевое разделение ответственности

### Arduino Uno — authoritative realtime/safety owner

- физический START;
- единоличное управление SSR;
- Hall sensor на `A0`;
- realtime threshold/hysteresis/direction и turn counting;
- winding state machine;
- LCD 1602;
- proven Keypad scan/debounce semantics;
- buzzer;
- минимальный CMP1 UART;
- EEPROM runtime Hall profile;
- EEPROM pending completed events и exact identifiers;
- local confirmation для calibration и apply.

### ESP32 — extended analysis/UI owner

- extended Hall test aggregation/analysis;
- threshold/hysteresis/direction recommendation;
- calibration UI;
- runtime calibration keepalive;
- proposal creation и audit/mirror data;
- orchestration без права START/SSR.

ESP32 не управляет SSR и не создаёт automatic physical START. После сохранения Hall profile обычная намотка должна работать автономно без ESP32.

## Полный целевой Hall calibration/apply handshake

```text
ESP32 CAL_ARM
  -> Arduino WAITING_LOCAL_CONFIRM
  -> operator # on Arduino
  -> Arduino ARMED_WAITING_START
  -> baseline sampling
  -> separate physical START
  -> Arduino RUNNING
  -> bounded Hall measurement summary
  -> ESP32 analyzes summary
  -> ESP32 creates CAL_PROPOSAL bound to exact source measurement identity/CRC
  -> Arduino validates proposal + identity + ranges + safe state
  -> Arduino waits for local operator confirmation
  -> Arduino atomically saves CRC/versioned profile to EEPROM
  -> Arduino CAL_APPLIED with actual persisted profile + identity
  -> ESP32 stores mirror/audit result
```

Правила:

- ESP32 CAL_ARM никогда не запускает двигатель;
- `#` и physical START — два разных локальных действия;
- SSR включается только Arduino и только при `HallCalibrationState::Running`;
- потеря ESP32/UART во время active calibration должна fail-close в `Aborted`/SSR OFF;
- recommendation/proposal не применяется автоматически;
- Arduino независимо проверяет ranges и exact proposal identity;
- authoritative profile хранится на Arduino EEPROM;
- ESP32 mirror не имеет права автоматически перезаписывать Arduino;
- reboot не продолжает calibration/proposal/apply;
- EEPROM не очищать;
- обычный production Hall counting автономен после сохранения.

## Что уже завершено

### Uno memory optimization

**Software/memory gate GREEN на `10e11bba`.**

- Keypad capacity уменьшена без смены proven scan algorithm;
- Wire/LiquidCrystal persistent buffers убраны из Uno production path;
- LCD работает через bufferless PCF8574 owner;
- Hall result/telemetry runtime уменьшены;
- worst-case 10-coil JOB receive boundary исправлен;
- 469 bytes static SRAM headroom подтверждены Actions.

### Hall analysis migration

- `CM_HallCalibrationAnalyzer` на ESP32 рассчитывает recommendation из summary;
- Arduino recommendation math удалён;
- UI показывает ESP32 recommendation;
- bounded telemetry ownership перенесена на ESP32 side.

### Hall arm/start safety

Implementation completed but **pending Actions verification**:

- explicit `WAITING_LOCAL_CONFIRM`;
- local `#` confirmation;
- separate physical START;
- ESP32 runtime keepalive;
- Arduino 3-second peer timeout abort;
- host regression contract updated.

## Следующий active block

1. Получить новый Uno build + host-tests после `f2b8d77e`; записать actual RAM/Flash и не объявлять GREEN раньше результата.
2. После GREEN завершить exact `CAL_PROPOSAL` measurement identity handshake.
3. Proposal не должен использовать generic `CFG_SET` как автоматический shortcut.
4. Добавить Arduino-side pending proposal owner без auto-apply и с local confirmation.
5. Atomically persist authoritative profile через существующий settings store/CRC semantics.
6. Добавить `CAL_APPLIED` exact identity response и ESP32 mirror/audit state.
7. Reboot должен забывать pending proposal, но загружать последний CRC-valid applied profile.
8. После software completion — единый hardware acceptance gate.

## Финальный hardware acceptance gate

Промежуточные hardware smoke-tests не выполнять. После завершения software migration проверить одним циклом:

1. Arduino boot/home + отсутствие reset-loop.
2. LCD 1602.
3. Keypad: `1`, `#`, `*`, `D`, emergency `D * # D`.
4. Hall manual rotation test без SSR.
5. `CAL_ARM -> # -> baseline -> physical START`.
6. UART disconnect during active calibration -> SSR OFF / ABORTED.
7. ESP32 analysis -> proposal -> local apply confirmation -> EEPROM persistence.
8. Reboot -> no calibration/apply resume; saved profile loads.
9. ESP32<->Arduino JOB receive; Arduino остаётся READY до physical START.
10. Только physical START создаёт `RUN_STARTED` и разрешает SSR по Arduino state.
11. Exact `RUN_COMPLETED` доставляется/retries до ACK.
12. Material writeoff остаётся manual exact `spool_id + source_session_id + source_run_id`.

## Safety invariants — не ослаблять

- no automatic physical START;
- no automatic START between repeat runs;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically writes off material;
- manual writeoff requires exact session + run + immutable spool;
- cancellation never deletes immutable history;
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

Продолжать только из `cmp-protocol-v1`. Сначала проверить Actions для latest Hall local-confirm/UART-loss block (`f2b8d77e` или более новый head), зафиксировать build/host status и actual Uno Flash/RAM. Hardware checks до окончания software migration не запрашивать. После GREEN реализовать exact `CAL_PROPOSAL -> local apply confirmation -> CAL_APPLIED` handshake с measurement identity, не передавая ESP32 право START/SSR и не очищая EEPROM. Перед каждым update fetch актуальный файл и current blob SHA. Для нового файла сначала подтвердить отсутствие пути. Не утверждать CI/build/hardware GREEN без фактического evidence.
