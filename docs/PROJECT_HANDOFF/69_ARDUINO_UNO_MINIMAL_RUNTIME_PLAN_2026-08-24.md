# CoilMaster — план минимального runtime Arduino Uno

Дата: **2026-08-24**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Назначение

Arduino Uno остаётся минимальным authoritative realtime/safety controller. ESP32 получает расширенный Hall analysis/UI, но не получает права physical START или SSR.

## Последний полностью verified software/memory baseline

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

Цель static SRAM headroom 350–400 bytes достигнута с запасом. В этот verified image уже входят bounded Keypad 3.1.1 semantics, bufferless PCF8574 LCD/TWI owner, compact Hall runtime, ESP32-owned recommendation math и 112-byte CMP1 RX boundary для worst-case 10-coil JOB.

**Важно:** цифры 469 B / 2596 B относятся к `10e11bba`. После нового Hall local-confirm/proposal/apply кода Uno размер нужно измерить новым Actions build. Не переносить эти цифры на текущий HEAD как уже подтверждённые.

## Подтверждённый промежуточный checkpoint

После исправления ESP32 Wire/LCD compatibility пользователь сообщил, что три новые CI проверки GREEN. На этой базе продолжен exact Hall apply handshake.

Последний prior infrastructure fix:

```text
e2f76d41165fb6a2fa9b845eaf11a4b17e23f25a
build(esp32): expose framework Wire through LCD compat shim
```

## Текущий Hall flow — реализован в коде, latest batch ещё требует Actions verification

### 1. Calibration arm / motor start

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

Правила:

- `CAL_ARM` никогда не запускает двигатель;
- physical START до первого `#` блокируется;
- любая неправильная keypad action во время calibration abort-ит flow;
- ESP32/Web не управляют SSR.

### 2. UART-loss fail-safe

ESP32 runtime посылает валидный `CMP1|CAL|GET|C|CRC` keepalive примерно раз в 1 s.

Arduino:

```text
PeerTimeoutMs = 3000 ms
```

При потере валидного CAL traffic active calibration/apply переводится в `Aborted`, transient proposal очищается, SSR остаётся OFF. Никакого auto-resume после reboot.

### 3. Measurement identity

Arduino возвращает measurement-only summary и transient non-zero `measurement_id`:

```text
CMP1|CAL_RESULT|INVALID|baseline|min|max|0|0|RISING|samples|duration_ms|measurement_id|C|CRC
```

Threshold/hysteresis/direction placeholders на Uno остаются нейтральными. Recommendation вычисляется только `CM_HallCalibrationAnalyzer` на ESP32.

`measurement_id` строится из конкретного arm/result context и не хранится как pending state в EEPROM. После reboot старый proposal нельзя продолжить.

### 4. Exact proposal

ESP32 отправляет:

```text
CMP1|CAL_PROPOSAL|measurement_id|threshold|hysteresis|release_debounce_ms|direction|C|CRC
```

Arduino до staging проверяет:

- CRC/shape;
- settings ranges;
- safe machine state;
- наличие текущего completed result;
- exact `measurement_id` equality.

Proposal переиспользует существующий bounded single-slot hardware-control request. Новая UART queue не создавалась.

### 5. Local confirmation перед EEPROM

После валидного proposal:

```text
Arduino WAITING_APPLY_CONFIRM
```

В этом состоянии:

- settings лежат только transient RAM;
- EEPROM ещё не изменён;
- SSR force OFF;
- physical START блокируется и **не** считается подтверждением;
- только отдельный `#` на Arduino вызывает существующий `HardwareSettingsController::apply()`;
- любая другая keypad action отменяет proposal;
- CAL abort / UART-loss / timeout отменяют proposal без записи.

### 6. Authoritative persistence / response

После локального `#` Arduino использует существующий CRC/versioned `HardwareSettingsStore` как единственный authoritative EEPROM owner и отвечает:

```text
CMP1|CAL_APPLIED|measurement_id|result|threshold|hysteresis|release_debounce_ms|direction|C|CRC
```

ESP32 принимает final response только если `measurement_id` совпадает с pending proposal id.

Допустимые result semantics включают:

```text
APPLIED
BUSY
INVALID
IDENTITY_MISMATCH
PERSISTENCE_FAILED
CANCELLED
```

### 7. Web/UI apply path

Calibration UI больше не использует generic `/api/hardware/hall/settings` для применения recommendation.

Новый endpoint:

```text
POST /api/hardware/hall/calibration/apply
```

Browser передаёт только `release_debounce_ms`.

ESP32 server сам берёт из текущего cached calibration result:

- exact `measurement_id`;
- recommended threshold;
- recommended hysteresis;
- recommended direction.

Это не позволяет браузеру подменить recommendation в calibration apply path.

UI показывает второй обязательный локальный шаг:

```text
WAITING_APPLY_CONFIRM
-> нажать # на Arduino для EEPROM save
```

START не подтверждает сохранение.

## Последние commits exact apply batch

```text
91b6b865  feat(hall): add exact calibration proposal client
f424d1b9  feat(hall): bind proposal to measurement identity
2b61a892  feat(hall): publish exact calibration measurement id
4f22725c  test(hall): require exact calibration proposal identity
d916f5c1  feat(hall): expose calibration proposal lane
2f3f667b  feat(hall): route exact calibration proposals on Uno
a8094872  fix(hall): keep proposal alive through local apply confirm
ace97936  feat(hall): require local confirm before calibration EEPROM apply
26d6d734  feat(hall): route calibration proposals through ESP32 receiver
7943806a  feat(hall): expose exact calibration apply endpoint
d403da97  feat(hall): stage exact calibration proposal from web
aea6042f  feat(hall): require Arduino confirm for calibration apply
d1acbe4b  test(hall): enforce local-confirmed exact apply path
```

Current expected HEAD at записи этого handoff:

```text
d1acbe4b14b38e4381063f9ce1beaa44005c7896
```

Всегда refetch branch HEAD перед изменением.

## Verification status

### GREEN / доказано

- memory optimization baseline `10e11bba`;
- 469 B free SRAM и 2596 B free Flash на этом baseline;
- bounded Keypad owner;
- bufferless LCD owner;
- physical START / Arduino SSR authority invariants в prior verified tests;
- пользователь сообщил 3 GREEN CI после `e2f76d41` infrastructure checkpoint.

### UNVERIFIED после exact apply batch

Нужно получить Actions именно на `d1acbe4b` или более новом descendant:

1. host-tests;
2. ESP32 PlatformIO build;
3. Uno PlatformIO build;
4. фактические Uno RAM/Flash после добавления transient proposal state.

До этого не объявлять latest exact apply software GREEN и не утверждать, что current Uno всё ещё имеет ровно 469 B free SRAM.

### Hardware

По решению пользователя промежуточные hardware smoke-tests не выполнять. Hardware gate остаётся отложенным до завершения software migration.

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

1. Проверить latest Actions для `d1acbe4b` или descendant.
2. Если compile/test failure — исправить точечно, не откатывая exact-id/local-confirm architecture.
3. Зафиксировать новую Uno SRAM/Flash цифру.
4. Проверить remote abort semantics в `WAITING_APPLY_CONFIRM`; local key/UART-loss cancellation уже fail-closed.
5. Добавить ESP32 mirror/audit representation final `CAL_APPLIED` profile, если текущего web reply cache недостаточно для reboot-visible audit.
6. Обновить `06_ACTIVE_WORK_AND_NEXT_STEPS.md` после software gate stabilization.
7. Только после software completion провести единый hardware acceptance.

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

Работать только из `cmp-protocol-v1`. Перед каждым update fetch актуальный файл и blob SHA. Hardware checks пока не запрашивать. Сначала получить CI/build/size evidence exact apply batch. После GREEN завершить audit/mirror и обновить `06`. Не передавать ESP32 право START/SSR, не делать auto-apply и не очищать EEPROM.
