# CoilMaster — UART Arduino Uno ↔ ESP32

## Физическое подключение через 8-канальный LLC

| Arduino Uno | Канал LLC | ESP32 |
|---|---|---|
| A1 (TX) | A1 → B1 | GPIO16 (RX2) |
| A2 (RX) | A2 ← B2 | GPIO17 (TX2) |
| GND | общий GND | GND |

Линии UART подключены перекрестно:

- Arduino TX → ESP32 RX;
- Arduino RX ← ESP32 TX.

## Уровни логики

- Arduino Uno: 5 В;
- ESP32: 3,3 В;
- между платами обязателен преобразователь логических уровней;
- прямое подключение 5-вольтового TX Arduino к GPIO ESP32 запрещено;
- все устройства должны иметь общий GND.

## Arduino Uno

`SoftwareSerial` принимает аргументы в порядке RX, TX:

```cpp
#include <SoftwareSerial.h>

constexpr uint8_t ESP_RX_PIN = A2;
constexpr uint8_t ESP_TX_PIN = A1;

SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN);
```

## ESP32

```cpp
constexpr int UART2_RX_PIN = 16;
constexpr int UART2_TX_PIN = 17;

Serial2.begin(57600, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);
```

## Параметры канала

- формат: 8N1;
- начальная рабочая скорость: 57600 бод;
- 115200 бод использовать только после отдельного длительного теста на Arduino Uno;
- транспортный формат данных: CMP Protocol.

## Проверка

Перед интеграцией прикладной логики необходимо выполнить двусторонний тест:

1. Uno отправляет тестовый пакет на ESP32;
2. ESP32 проверяет CRC и отвечает;
3. Uno проверяет ответ;
4. тест повторяется серией пакетов без потерь и ошибок CRC.
