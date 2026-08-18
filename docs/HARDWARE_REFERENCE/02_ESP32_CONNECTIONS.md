# ESP32 — подключения CoilMaster

Дата: **2026-08-18**  
Проверено по production entrypoint `firmware/esp32/src/main.cpp`.

## Полная таблица подключений

| Модуль / сигнал | GPIO ESP32 | Назначение |
|---|---:|---|
| microSD CS | GPIO5 | Chip Select SPI microSD |
| microSD SCK | GPIO18 | SPI clock |
| microSD MISO | GPIO19 | данные microSD → ESP32 |
| microSD MOSI | GPIO23 | данные ESP32 → microSD |
| RTC DS3231 SDA | GPIO21 | I²C SDA |
| RTC DS3231 SCL | GPIO22 | I²C SCL |
| UART RX2 от Arduino | GPIO16 | ESP32 RX ← Arduino A1 TX через LLC |
| UART TX2 к Arduino | GPIO17 | ESP32 TX → Arduino A2 RX через LLC |

## microSD / флеш-карта

В проекте рабочее хранилище — SPI microSD.

Подключение:

```text
microSD CS   -> GPIO5
microSD SCK  -> GPIO18
microSD MISO -> GPIO19
microSD MOSI -> GPIO23
microSD GND  -> GND
```

Питание microSD подключать согласно конкретному модулю. Если модуль рассчитан на питание 5 V и имеет встроенный стабилизатор/level shifting, использовать его штатную схему. Если это голый 3.3 V модуль, 5 V подавать нельзя.

На карте хранятся production данные, web assets, журналы, backup и конфигурация. Автоматическое удаление production data при заполнении карты запрещено safety-контрактом.

## RTC DS3231

I²C подключение:

```text
DS3231 SDA -> GPIO21
DS3231 SCL -> GPIO22
DS3231 GND -> GND
```

Питание RTC — согласно используемому модулю.

## UART ESP32 ↔ Arduino

Используется HardwareSerial(2), скорость:

```text
9600 baud
```

Соединение:

```text
ESP32 GPIO16 RX2 <- LLC <- Arduino A1 TX
ESP32 GPIO17 TX2 -> LLC -> Arduino A2 RX
ESP32 GND        -------- Arduino GND
```

TX всегда соединяется с RX противоположной платы.

Из-за 3.3 V logic ESP32 и 5 V logic Arduino в фактической сборке используется преобразователь логических уровней (`LLC`).

## Что не подключено к ESP32 напрямую

Следующие исполнительные/локальные элементы принадлежат Arduino, а не ESP32:

- SSR;
- Hall SS49E;
- зуммер;
- физическая START-кнопка;
- клавиатура 4×4;
- LCD1602.

ESP32 отвечает за Web, хранение, RTC, microSD и обмен заданиями/событиями по UART. Она не должна напрямую включать SSR и не выполняет physical START.
