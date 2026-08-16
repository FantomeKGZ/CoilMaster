# CoilMaster

CoilMaster — локальная система управления намоточным станком, учёта ремонтов,
двигателей, материалов и резервных копий.

Единственная рабочая ветка исходников: `cmp-protocol-v1`. Ветка `main` не
используется как источник реализации.

## Контроллеры

- Arduino Uno выполняет realtime-логику намотки, считает импульсы датчика
  Холла и только после физического подтверждения управляет SSR.
- ESP32 обслуживает Wi-Fi, HTTP, FTP, microSD, RTC, базы данных, журналирование
  и передачу заданий Arduino.

ESP32 и Web не управляют SSR напрямую. После перезагрузки нет автоматического
продолжения или запуска. `RUN_COMPLETED` не списывает провод автоматически.

## Актуальная структура

```text
Arduino/                    модули Arduino Uno и аппаратная конфигурация
Core/                       общая realtime-модель намотки Uno
firmware/arduino/src/       production entry point Arduino
firmware/esp32/src/         production firmware ESP32
firmware/esp32/web/         desktop/mobile web-интерфейс для microSD /web
Engineering/Hardware/       pin-map, питание и соединения плат
docs/                       спецификации реализованных функций
docs/PROJECT_HANDOFF/       текущее состояние и точка продолжения
Shared/Protocol/            ранний бинарный CMP, пока только host-тесты
Tests/Protocol/             host-тесты раннего бинарного CMP
Tests/Web/                  аудит web assets, ссылок и интерфейса
```

Рабочий UART-контракт Arduino ↔ ESP32 сейчас использует строковые кадры
`CMP1|...`. Он реализован в `Arduino/CM_UartEventTransport.*` и
`firmware/esp32/src/CM_UartEventReceiver.*`. `Shared/Protocol/` нельзя считать
production-интеграцией до отдельной унификации.

## Сборка

```text
pio run -e uno
pio run -e esp32
```

ESP32 использует разметку `huge_app.csv`; полный web-каталог после обновления
копируется на microSD в `/web`.

Перед продолжением разработки читать `docs/PROJECT_HANDOFF/00_READ_FIRST.md` и
`docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
