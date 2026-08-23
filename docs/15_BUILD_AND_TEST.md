# CoilMaster — сборка и безопасная первичная проверка Arduino Uno

## 1. Authoritative build composition

Текущий Arduino Uno production entrypoint:

```text
firmware/arduino/src/main.cpp
```

`platformio.ini` собирает только:

```text
Core/*.cpp
Arduino/*.cpp
firmware/arduino/src/main.cpp
```

Старый `Arduino/CoilMaster_Arduino.ino` удалён во время controlled cleanup и **не должен быть восстановлен как второй production entrypoint**.

Команда локальной сборки:

```bash
pio run -e uno
```

Applicable GitHub Actions gate: **Arduino Uno Build**. CMP/UART/state-machine changes также требуют соответствующих protocol/contract gates. CI GREEN не является hardware GREEN.

## 2. Current feature configuration

Authoritative compile-time switches:

```text
Arduino/Config/CM_Features.h
```

Normal production configuration включает LCD, keypad, external START, Hall, SSR, buzzer, ESP32 UART и calibration. Verbose human-readable diagnostics сейчас отключены для экономии ATmega328P flash; CMP1/safety/START/SSR от этого не зависят.

Simulation в normal machine firmware должна оставаться:

```cpp
#define CM_FEATURE_SIMULATION 0
```

Если simulation временно включается для отдельной bench-проверки, `CM_SIMULATION_BLOCK_REAL_SSR` обязан блокировать реальный SSR. Перед эксплуатацией simulation возвращается в `0`.

## 3. Current Arduino pins

Authoritative map:

```text
Arduino/Config/CM_Pins.h
```

Текущие основные назначения:

```text
Keypad rows       D2..D5
Keypad columns    D6..D9
Physical START    D10
SSR control       D12
Hall SS49E        A0
ESP32 UART TX     A1 -> level converter -> ESP32 GPIO16 RX2
ESP32 UART RX     A2 <- level converter <- ESP32 GPIO17 TX2
Buzzer            A3
LCD1602 I2C       A4/A5 (standard Uno I2C pins)
UART baud          9600
```

Не использовать старые документы/схемы, где buzzer указан на D11 или production UART отсутствует.

## 4. Безопасность первого аппаратного запуска

Первичную functional проверку выполнять **без запитанной силовой нагрузки SSR**. Низковольтную управляющую часть можно проверять только после сверки проводки и общего GND.

Перед подачей питания проверить:

- общий GND низковольтных модулей;
- `CM_Pins.h` против фактической проводки;
- level shifting между Arduino и ESP32 UART;
- SSR control wiring на D12;
- Hall на A0;
- physical START на D10;
- buzzer на A3;
- I2C LCD;
- отсутствие питания силовой нагрузки во время bench smoke.

Не использовать Web/ESP32 как замену physical START и не подключать силовую часть только потому, что firmware build прошёл.

## 5. Boot smoke без силовой нагрузки

После boot проверить минимум:

1. SSR command остается безопасно выключенной до разрешенного physical START.
2. UI/input работают без spontaneous movement command.
3. Remote JOB может быть принят/показан, но прием JOB/ACK не запускает SSR.
4. Physical START остается единственным разрешенным boundary для начала winding movement.
5. Pause/cancel/menu/recovery paths не создают automatic restart.
6. После reboot winding автоматически не возобновляется.
7. Repeat не запускает следующий физический run автоматически.

Если один из пунктов не выполняется, силовую нагрузку не подключать.

## 6. Hall smoke

С отключенной силовой нагрузкой проверить Hall сигнал контролируемым магнитом/тестовым вращением:

- одна корректная метка не должна превращаться в множественные витки;
- threshold/hysteresis берутся из persisted/current hardware settings;
- calibration может быть armed через service flow, но движение во время calibration все равно требует physical START;
- Web calibration command не должен сам включать SSR.

При нестабильном Hall сначала исправить sensor/wiring/calibration; не компенсировать ошибку автоматическим движением.

## 7. UART two-board smoke

Когда требуется аппаратная интеграционная проверка ESP32 <-> Arduino:

1. загрузить current `cmp-protocol-v1` firmware на обе платы;
2. убедиться, что UART подключен через level converter по `CM_Pins.h`;
3. доставить один test JOB;
4. подтвердить, что до physical START движения нет;
5. выполнить один controlled run;
6. проверить `RUN_STARTED` и `RUN_COMPLETED` с одним exact `run_id`;
7. при необходимости проверить bounded ACK/retry без создания duplicate run evidence;
8. проверить повтор: новый physical START -> новый `run_id`;
9. проверить reboot: automatic resume отсутствует.

Runtime/Serial capture нужен только для конкретного unresolved hardware-only дефекта, а не как обязательный шум для каждого source cleanup commit.

## 8. Material boundary после run

Даже успешный hardware `RUN_COMPLETED` не должен менять warehouse stock автоматически.

Для current linked production последующий writeoff остается отдельным manual action с:

```text
source_session_id + source_run_id + exact immutable spool_id
```

Это проверяется отдельно от winding smoke.

## 9. Критерии допуска к силовому тесту

Перед подключением силовой нагрузки должны быть подтверждены как минимум:

- current Uno build проходит для тестируемого SHA;
- simulation отключена;
- фактические pins соответствуют `CM_Pins.h`;
- boot/reboot не создают automatic START;
- SSR остается OFF вне разрешенного Arduino state;
- physical START boundary работает локально;
- Hall count стабилен;
- repeat не auto-start;
- ESP32/Web не имеют direct SSR ownership;
- UART acceptance не эквивалентен START.

Силовой hardware acceptance выполняется оператором отдельно и никогда не выводится только из CI.
