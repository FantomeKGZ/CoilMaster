# CoilMaster — аппаратный справочник

Дата: **2026-08-20**  
Source of truth: `cmp-protocol-v1`

Эта папка предназначена как быстрый практический справочник по физическому подключению CoilMaster, управлению Arduino с клавиатуры и калибровке доступного оборудования.

## Что читать

1. `01_ARDUINO_CONNECTIONS.md` — все подключения Arduino Uno: клавиатура, START, зуммер, SSR, Hall, LCD1602 и UART к ESP32.
2. `02_ESP32_CONNECTIONS.md` — подключения ESP32: microSD, RTC DS3231 и UART к Arduino.
3. `03_KEYS_AND_HIDDEN_COMMANDS.md` — назначение клавиш, внешней START и скрытая аварийная комбинация.
4. `04_RTC_TIME_SYNC.md` — DS3231, ручная установка времени, диагностика ошибок и автоматическая NTP-синхронизация времени Кыргызстана / Бишкек.
5. `05_FTP_WEB_RECOVERY.md` — восстановительный FTP `/web`, AP/STA access и safe-idle ограничения, если файл присутствует в текущей ветке.
6. `06_FIRMWARE_IDENTIFICATION.md` — идентификация реально запущенной ESP32 firmware/build, если файл присутствует в текущей ветке.
7. `07_HALL_SENSOR_CALIBRATION.md` — SS49E: baseline `590/50`, live ADC calibration, stable-release защита и hardware tests против повторного счёта при зависшем магните.

## Hall — текущий baseline

```text
Arduino A0
threshold = 590
hysteresis = 50
release threshold = 540
```

Старый рабочий скетч пользователя показывал ориентировочно около `522` без магнита и `660` с магнитом. Это референс, а не универсальное значение: реальная установленная система должна калиброваться по live ADC.

Новый planned UI должен показывать current/min/max ADC, magnet state, threshold/hysteresis/release debounce и позволять оператору фиксировать диапазоны `покой` / `магнит`, после чего вручную подтвердить предложенные настройки.

## Важное правило

Если таблица в этой папке расходится с кодом, источником истины остаётся текущий код ветки `cmp-protocol-v1`, прежде всего:

```text
Arduino/Config/CM_Pins.h
Arduino/CM_HallTurnSource.h
Arduino/CM_HallTurnSource.cpp
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
- `RUN_COMPLETED` не списывает провод автоматически;
- Hall/hardware settings изменяются только при доказанном safe idle;
- calibration UI не включает SSR и не запускает двигатель.
