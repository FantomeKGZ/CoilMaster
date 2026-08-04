# CoilMaster — аппаратная документация

Этот каталог содержит подтвержденную распиновку и правила подключения текущей сборки CoilMaster.

## Документы

- [Arduino Uno: полная карта выводов](Arduino_Uno_Pinout.md)
- [ESP32: подтвержденные подключения](ESP32_Pinout.md)
- [UART Arduino Uno ↔ ESP32](UART_Uno_ESP32.md)
- [Питание через LM2596](Power_LM2596.md)

## Текущая архитектура

```text
LM2596, линия 5 В
        |
        +-- Arduino Uno
        |     +-- LCD1602 I2C
        |     +-- SS49E
        |     +-- Keypad 4x4
        |     +-- START
        |     +-- Buzzer
        |     +-- SSR control
        |
        +-- ESP32
              +-- DS3231, I2C GPIO21/GPIO22
              +-- UART2 GPIO16/GPIO17

Arduino Uno <-- 8-channel LLC --> ESP32
```

## Статусы

### Подтверждено

- полная распиновка Arduino Uno;
- UART Uno ↔ ESP32;
- ESP32 UART2 GPIO16/GPIO17;
- ESP32 I²C GPIO21/GPIO22 для DS3231;
- LM2596 как источник линии 5 В;
- общий GND всех логических модулей.

### Ожидает повторного подтверждения

- точная распиновка Nextion;
- точная распиновка microSD;
- питание Nextion и microSD;
- дополнительные модули ESP32;
- точная модель платы ESP32;
- входное напряжение и ток источника LM2596.

Неподтвержденные выводы не должны использоваться в прошивке как окончательные.
