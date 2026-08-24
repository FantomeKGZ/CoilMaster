# CoilMaster — план минимального runtime Arduino Uno

Дата: **2026-08-24**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Назначение

Arduino Uno остаётся минимальным authoritative realtime/safety controller. ESP32 получает расширенный Hall analysis/UI, но не получает права physical START или SSR.

## Последний полностью verified pre-exact-apply baseline

Commit:

```text
10e11bba081d06eb3b149eb9c2bd7b593faa8895
```

Actions:

```text
32698376710  Arduino Uno build  SUCCESS
32698376682  host-tests         SUCCESS
```

Размер:

```text
RAM   1579 / 2048
free SRAM = 469 bytes

Flash 29660 / 32256
free Flash = 2596 bytes
```

В этот image уже входят bounded Keypad 3.1.1 semantics, bufferless PCF8574 LCD/TWI owner, compact Hall runtime, ESP32-owned recommendation math и 112-byte CMP1 RX boundary для worst-case 10-coil JOB.

## Exact Hall apply — software architecture

### Calibration / motor start

```text
ESP32 CAL_ARM
  -> Arduino WAITING_LOCAL_CONFIRM
  -> operator # on Arduino
  -> Arduino ARMED_WAITING_START
  -> bounded baseline sampling
  -> separate physical START
  -> Arduino RUNNING
  -> SSR permit только Arduino и только в RUNNING
```

`CAL_ARM` никогда не запускает двигатель. START до первого `#` блокируется. ESP32/Web не управляют SSR.

### UART-loss fail-safe

ESP32 посылает `CAL|GET` keepalive примерно раз в 1 s. Arduino использует `PeerTimeoutMs = 3000 ms`. Потеря валидного CAL traffic во время calibration/apply приводит к abort; transient proposal удаляется, SSR остаётся OFF. Reboot ничего не продолжает автоматически.

### Exact measurement identity

Arduino возвращает measurement-only result:

```text
CMP1|CAL_RESULT|INVALID|baseline|min|max|0|0|RISING|samples|duration_ms|measurement_id|C|CRC
```

Recommendation вычисляется только ESP32. `measurement_id` transient и не записывается в EEPROM.

### Exact proposal / local apply

ESP32:

```text
CMP1|CAL_PROPOSAL|measurement_id|threshold|hysteresis|release_debounce_ms|direction|C|CRC
```

Arduino проверяет CRC/shape, settings ranges, safe state, completed measurement и exact identity. Proposal использует существующий bounded single-slot hardware-control request.

После валидного proposal:

```text
WAITING_APPLY_CONFIRM
```

В этом состоянии settings находятся только в RAM; SSR OFF; START блокируется; только отдельный локальный `#` вызывает `HardwareSettingsController::apply()`.

После apply Arduino отвечает authoritative profile:

```text
CMP1|CAL_APPLIED|measurement_id|result|threshold|hysteresis|release_debounce_ms|direction|C|CRC
```

Допустимые result semantics: `APPLIED`, `BUSY`, `INVALID`, `IDENTITY_MISMATCH`, `PERSISTENCE_FAILED`, `CANCELLED`.

EEPROM owner остаётся существующий CRC/versioned `HardwareSettingsStore`; proposal/measurement identity в EEPROM schema отсутствуют.

### Web apply

Calibration UI не использует generic `/api/hardware/hall/settings` для recommendation. Endpoint:

```text
POST /api/hardware/hall/calibration/apply
```

Browser передаёт только release debounce; exact measurement id и recommendation ESP32 берёт из текущего analyzer result. EEPROM save всё равно требует отдельный `#` на Arduino.

## Exact-apply size regression и recovery

После добавления полного exact-id/local-confirm apply flow первый Uno size gate на commit `b2ce03c7` дошёл до линковки, но не поместился:

```text
Actions 32707416029  build-uno  FAILED (size)
RAM   1757 / 2048  -> free 291 bytes
Flash 32708 / 32256 -> overflow 452 bytes
```

Функциональная compile/link логика была корректна; failure был только `checkprogsize`.

Затем выполнен size-recovery без удаления safety-функций:

```text
189990f8  remove duplicate Hall proposal parser path
4a11a7d6  hide obsolete hardware-control name API
913ee57e  move hardware protocol literals to AVR PROGMEM; canonical numeric parser
 eaa1b93f test(hall): enforce single proposal parser owner
```

### Последний verified Uno size gate

Actions:

```text
32708164073  build-uno  SUCCESS
```

Exact checkout:

```text
913ee57e723bac0ca71bbdca7cddf9bc0ce699ed
```

Размер:

```text
RAM   1219 / 2048 = 59.5%
free SRAM = 829 bytes

Flash 32144 / 32256 = 99.7%
free Flash = 112 bytes
```

SRAM gate теперь имеет очень хороший запас. Flash compile GREEN, но **112 B headroom считается слишком малым для завершённого production gate**. Продолжается только Flash-size recovery без удаления exact-id/local-confirm safety semantics.

### Последний verified host gate

Actions:

```text
32708236369  host-tests  SUCCESS
```

Exact checkout:

```text
eaa1b93f2fcbd0579503aeedd93a62baddc72fe7
```

Подтверждено:

- 4/4 CMake tests;
- release safety contracts;
- physical START / Arduino SSR authority;
- exact-id/local-confirm Hall contracts;
- transient proposal persistence boundary;
- single authoritative `CAL_PROPOSAL` parser owner;
- no automatic material writeoff.

Ранние runs `32708072646`, `32708120077`, `32708164026` были промежуточными stale Hall audit failures до `eaa1b93f`. `32708119989` был промежуточным compile failure на `4a11a7d6` до завершения PROGMEM helper refactor. Они не являются состоянием текущего verified checkpoint.

## Current Flash recovery after verified 112 B headroom

После `eaa1b93f` начат следующий безопасный подпакет:

```text
2a64c818  expose shared hardware CRC frame validator
 a240a70c share CRC validator implementation owner
 b168cf75 reuse hardware CRC validator for Hall CAL
20e78589  trim stale Hall calibration protocol declarations
```

Hall `CAL ARM/ABORT/GET` больше не должен держать второй независимый `parseHex16 + CRC verify` implementation. `CAL_PROPOSAL` уже проходит через authoritative `HardwareControlProtocol::parseRequest()`.

**Этот новый CRC-sharing подпакет ещё требует нового Uno build/size evidence.** Не переносить цифры 829/112 на его HEAD как уже измеренные.

Если Flash headroom после него всё ещё недостаточен, следующий безопасный кандидат — перевести `CAL_STATE`, `CAL_RESULT`, `CAL_APPLIED` на уже существующий streaming `CrcFrameWriter` в `CM_UartEventTransport` и удалить buffer-format Hall response layer. Wire bytes и protocol semantics должны остаться теми же.

## ESP32 mirror / compile status

ESP32 `processCalibrationApplied()` уже проверяет exact pending measurement id и парсит фактически сохранённые settings, но отдельный runtime mirror update из успешного `CAL_APPLIED` ещё нужно закрыть после стабилизации Uno Flash.

Свежего ESP32 PlatformIO build после полного exact-apply/size-recovery batch среди последних присланных runs **нет**. Не объявлять ESP32 compile GREEN без отдельного `build-esp32` evidence.

## Hardware policy

По решению пользователя промежуточные hardware smoke-tests не выполнять. Hardware gate остаётся отложенным до завершения software migration/optimization.

## Safety invariants — не ослаблять

- no automatic physical START;
- no automatic START between repeat runs;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- calibration proposal never auto-applies;
- EEPROM apply требует local `#` на Arduino;
- START не подтверждает EEPROM apply;
- lost UART during active calibration/apply => fail closed / SSR OFF;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically writes off material;
- manual writeoff requires exact session + run + immutable spool;
- cancellation never deletes immutable history;
- EEPROM pending completed events не очищать автоматически.

## Следующий active block

1. Получить Uno build/size после CRC-sharing batch (`20e78589` или descendant).
2. Цель: сохранить SRAM >= 350–400 B free и вернуть практический Flash headroom существенно выше 112 B.
3. Если Flash всё ещё близок к лимиту — streaming Hall response frames через существующий `CrcFrameWriter`, без изменения wire semantics.
4. После стабилизации Uno size закрыть ESP32 mirror фактически сохранённого `CAL_APPLIED` profile.
5. Получить отдельный `build-esp32` GREEN.
6. Обновить `06_ACTIVE_WORK_AND_NEXT_STEPS.md` после software gate stabilization.
7. Только затем единый hardware acceptance.

## Финальный hardware acceptance gate

После завершения software migration проверить одним циклом:

1. Arduino boot/home без reset-loop.
2. LCD 1602.
3. Keypad `1`, `#`, `*`, `D`, emergency `D * # D`.
4. Hall manual rotation без SSR.
5. `CAL_ARM -> # -> baseline -> physical START`.
6. UART disconnect during active calibration -> `ABORTED`, SSR OFF.
7. ESP32 result -> exact proposal -> `WAITING_APPLY_CONFIRM` -> local `#` -> EEPROM save.
8. START в `WAITING_APPLY_CONFIRM` ничего не сохраняет и не запускает.
9. Reboot -> no calibration/proposal resume; последний CRC-valid applied Hall profile загружается.
10. Remote JOB остаётся READY до physical START.
11. Только physical START создаёт `RUN_STARTED` и разрешает SSR по Arduino state.
12. Exact `RUN_COMPLETED` retries до ACK.
13. Material writeoff остаётся manual exact `spool_id + source_session_id + source_run_id`.

## Порядок чтения в новом чате

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

## Инструкция продолжения

Работать только из `cmp-protocol-v1`. Перед каждым update fetch актуальный файл и blob SHA. Hardware checks пока не запрашивать. Сначала получить новый Uno size после CRC-sharing batch; при необходимости продолжить streaming Hall frame optimization. Затем закрыть ESP32 mirror + build. Не передавать ESP32 право START/SSR, не делать auto-apply и не очищать EEPROM.
