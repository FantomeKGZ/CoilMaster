# CoilMaster — аппаратный справочник

Дата: **2026-08-18**  
Source of truth: `cmp-protocol-v1`

Эта папка предназначена как быстрый практический справочник по физическому подключению CoilMaster и управлению Arduino с клавиатуры.

## Что читать

1. `01_ARDUINO_CONNECTIONS.md` — все подключения Arduino Uno: клавиатура, START, зуммер, SSR, Hall, LCD1602 и UART к ESP32.
2. `02_ESP32_CONNECTIONS.md` — подключения ESP32: microSD, RTC DS3231 и UART к Arduino.
3. `03_KEYS_AND_HIDDEN_COMMANDS.md` — назначение клавиш, внешней START и скрытая аварийная комбинация.
4. `04_RTC_TIME_SYNC.md` — DS3231, ручная установка времени, диагностика ошибок и автоматическая NTP-синхронизация времени Кыргызстана / Бишкек.

## Важное правило

Если таблица в этой папке расходится с кодом, источником истины остаётся текущий код ветки `cmp-protocol-v1`, прежде всего:

```text
Arduino/Config/CM_Pins.h
Core/CM_KeyMapper.cpp
firmware/arduino/src/main.cpp
firmware/esp32/src/main.cpp
```

`main` не использовать как источник реализации.

## Safety

- physical START выполняется только физической клавишей `A` или внешней кнопкой START;
- ESP32/Web не управляет SSR напрямую;
- после reboot нет automatic resume;
- аварийная комбинация очистки задания не означает `RUN_COMPLETED` и не запускает двигатель;
- `RUN_COMPLETED` не списывает провод автоматически.
