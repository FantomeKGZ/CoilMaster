# CoilMaster — актуальная архитектура

Ветка реализации: `cmp-protocol-v1`.

## Граница ответственности

### Arduino Uno

- физический START и realtime state machine;
- Hall, LCD1602, keypad, buzzer и SSR;
- EEPROM recovery без автоматического возобновления движения;
- приём задания и отправка событий по UART.

### ESP32

- Wi-Fi/AP, mDNS, HTTP и ограниченный FTP;
- microSD, RTC, web UI и API;
- клиенты, двигатели, ремонты, склад, расчёты и отчёты;
- доставка задания Arduino, журнал событий и резервные копии.

ESP32/Web не имеют прямого пути управления SSR. Физический START остаётся
локальным. Завершённое UART-событие само по себе не списывает провод.

## Production flow

```text
client → motor → OPEN repair → costing → linked winding → exact spool
→ immutable snapshot/spool selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual exact-run writeoff
→ finalization preflight → CLOSED → reports → backup
```

Ручное списание связано с точными `spool_id`, `source_session_id` и
`source_run_id`.

## Исходники

```text
Core/                       realtime domain model для Uno
Arduino/                    аппаратные адаптеры и UART Uno
firmware/arduino/src/       production main Uno
firmware/esp32/src/         production modules ESP32
firmware/esp32/web/         сайт microSD
docs/PROJECT_HANDOFF/       authoritative handoff
Engineering/Hardware/       hardware reference
Tests/Web/                  web audit
```

`platformio.ini` явно выбирает production sources. Каталоги с другой
капитализацией не являются альтернативными исходниками.

## UART/CMP

Рабочий транспорт — ограниченные ASCII-кадры `CMP1|...` с CRC16 и строгой
проверкой полей. Реализации сторон:

- `Arduino/CM_UartEventTransport.*`;
- `firmware/esp32/src/CM_UartEventReceiver.*`.

`Shared/Protocol/` содержит ранний бинарный CMP (`0xAA55`, бинарный header,
CRC-CCITT) и пока используется только `Tests/Protocol/`. Его нельзя подключать
к production firmware без отдельной миграции и проверки SRAM Arduino Uno.

## Данные и восстановление

Authoritative данные хранятся на microSD в `/data`; web assets — в `/web`.
Постоянные журналы валидируются fail-closed. Восстановление не запускается после
перезагрузки автоматически. Любое будущее применение backup должно оставаться
операторским, транзакционным и иметь проверенную локальную rollback-копию.

Подробные API, storage invariants, hardware checkpoints и актуальные следующие
шаги находятся в `docs/PROJECT_HANDOFF/`.
