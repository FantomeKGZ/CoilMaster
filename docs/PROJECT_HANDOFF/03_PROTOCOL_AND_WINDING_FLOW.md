# UART-протокол и рабочий цикл намотки

## Актуальная реализация

Ветка `cmp-protocol-v1` использует строковые кадры протокола с префиксом `CMP1`.

Перед изменением формата всегда проверять текущие файлы:

```text
firmware/esp32/src/CM_UartEventReceiver.h
firmware/esp32/src/CM_UartEventReceiver.cpp
firmware/esp32/src/CM_WindingJournal.h
firmware/esp32/src/CM_WindingJournal.cpp
```

Старые документы о бинарном пакете CMP могут относиться к другой ветке или более ранней архитектуре и не должны автоматически применяться к этому коду.

## Задание ESP32 → Arduino

Фактический кадр формируется как:

```text
CMP1|JOB|<JOB_ID>|<SESSION_ID>|<TYPE>|<COIL_COUNT>|<TURNS>|<CRC16>
```

Где:

- `JOB_ID` — ненулевой идентификатор задания;
- `SESSION_ID` — ненулевой идентификатор сессии;
- `TYPE` — `WORKING` или `STARTING`;
- `COIL_COUNT` — от 1 до 10;
- `TURNS` — список чисел через запятую;
- число витков каждой катушки — от 1 до 9999;
- CRC вычисляется реализацией `UartEventReceiver::crc16()`.

Основная структура:

```cpp
CM::OutgoingWindingJob
```

## Подтверждение Arduino → ESP32

Текущая реализация принимает кадр вида:

```text
CMP1|JOB_ACK|<JOB_ID>|<STATUS>|<DETAIL>
```

Допустимые статусы:

```text
ACCEPTED
REJECTED
```

Для `REJECTED` поле `DETAIL` обязательно.

Поле детали ограничено по длине и допускает только:

- `A-Z`;
- `0-9`;
- `_`;
- `-`.

Неизвестный статус или неправильный `JOB_ID` не завершает ожидающую доставку.

## Повторная доставка задания

Текущая политика ESP32:

```text
Интервал повторной отправки: 2000 мс
Максимальное число отправок: 5
```

Результаты доставки:

```cpp
JobDeliveryResult::Accepted
JobDeliveryResult::Rejected
JobDeliveryResult::TimedOut
JobDeliveryResult::Cancelled
```

`JobDeliveryEvent` содержит:

- результат;
- `jobId`;
- число попыток;
- строку детали.

Новое задание нельзя поставить в очередь, пока предыдущий результат не извлечён через `takeJobDelivery()`.

`job_id` должен возрастать. Повторное или меньшее значение после ранее поставленного задания отклоняется.

Ожидающее задание можно отменить через:

```cpp
cancelPendingJob()
```

Отмена прекращает повторы передачи, но сама по себе не является удалённой командой остановки уже запущенного физического оборудования.

## События Arduino → ESP32

Формат события:

```text
CMP1|EVT|<TYPE>|<SESSION_ID>|<RUN_ID>|<COMPLETED_RUNS>|<CRC16>
```

Поддерживаемые типы:

```text
RUN_STARTED
RUN_COMPLETED
```

Правила разбора:

- `SESSION_ID` и `RUN_ID` — строгие десятичные `uint32_t`, больше нуля;
- знаки `+` и `-`, суффиксы и переполнение запрещены;
- `COMPLETED_RUNS` должен помещаться в `uint16_t`;
- `RUN_STARTED` требует `COMPLETED_RUNS = 0`;
- `RUN_COMPLETED` требует `COMPLETED_RUNS > 0`;
- лишние поля запрещены;
- CRC должен совпадать.

## Ответы ESP32 на события

Используются методы:

```cpp
sendAck(runId, status)
sendNack(runId, reason)
```

Форматы:

```text
CMP1|ACK|<RUN_ID>|<STATUS>
CMP1|NACK|<RUN_ID>|<REASON>
```

Перед расширением этих ответов нужно проверить совместимость с Arduino.

## Журнал намотки

Файл журнала:

```text
/data/winding-runs/events.ndjson
```

Текущая запись содержит:

```json
{
  "schema_version": 1,
  "run_id": 1,
  "event": "RUN_STARTED",
  "session_id": 10,
  "completed_runs": 0,
  "uptime_ms": 12345
}
```

Журнал защищает от следующих ошибок:

- повторное сохранение одинакового события;
- `RUN_COMPLETED` без ранее сохранённого `RUN_STARTED`;
- несовпадение сессии запуска и завершения;
- нулевые идентификаторы;
- неправильный счётчик `completed_runs`;
- два одновременных активных запуска в одной сессии;
- завершение неактивного запуска;
- повторное или уменьшающееся значение `run_id` в одной сессии.

## Правила последовательности

Для одной сессии нормальный порядок выглядит так:

```text
RUN_STARTED(run_id=1, completed_runs=0)
RUN_COMPLETED(run_id=1, completed_runs=1)
RUN_STARTED(run_id=2, completed_runs=0)
RUN_COMPLETED(run_id=2, completed_runs=2)
```

Нельзя принимать:

```text
RUN_COMPLETED без RUN_STARTED
RUN_STARTED второго run до завершения первого
RUN_COMPLETED другой session_id
RUN_STARTED с run_id <= уже использованного
скачок completed_runs с 1 сразу на 3
```

## Физический рабочий цикл

1. ESP32 подготавливает программу.
2. ESP32 передаёт задание Arduino.
3. Arduino проверяет возможность принять задание.
4. Arduino отвечает `ACCEPTED` или `REJECTED`.
5. Принятое задание не включает SSR автоматически.
6. Оператор физически подтверждает старт первой катушки.
7. Arduino отправляет `RUN_STARTED`.
8. Arduino выполняет счёт витков и управление SSR.
9. Arduino безопасно отключает SSR.
10. Arduino отправляет `RUN_COMPLETED`.
11. ESP32 сохраняет событие.
12. Для следующей катушки снова требуется физический START.
13. После последней катушки программа получает финальный статус.

## Текущее ограничение событий

`RemoteWindingEvent` пока содержит только:

- тип события;
- `sessionId`;
- `runId`;
- `completedRuns`.

В нём пока отсутствуют:

- `job_id`;
- `repair_id`;
- номер катушки программы;
- целевое число витков;
- фактическое число витков;
- материал и диаметр провода;
- снимок программы;
- причина остановки;
- источник задания.

Поэтому текущий журнал подтверждает последовательность запусков, но ещё не является полным производственным журналом ремонта.

## Граница Shared/Protocol — 2026-08-16

`Shared/Protocol/` является ранним binary CMP и участвует только в host tests;
production PlatformIO builds его не подключают. Рабочие `CMP1|...` transport
classes Arduino и ESP32 реализованы отдельно и обе используют CRC16/MODBUS
(initial `0xFFFF`, polynomial `0xA001`). Это несовместимо с binary CMP
CRC-CCITT `0x1021`. Поэтому существующий Shared core нельзя просто добавить в
build. На момент этого checkpoint описание CRC-CCITT в
`docs/17_UART_EVENT_TRANSPORT.md` было устаревшим; исправление зафиксировано
ниже.

## Общий CRC рабочего CMP1 — 2026-08-16

Commit `de8ee6b5da6b68d0880884e75f04e39e79c6b66d` устранил дублирование
CRC16/MODBUS в production transport classes. Arduino и ESP32 теперь используют
один stateless header `Shared/CMP1Text/CM_Cmp1Crc.h` без очередей, буферов и
динамической памяти. Binary `Shared/Protocol/` не изменён и остаётся отдельным
host-test-only форматом.

Добавлены прямые проверки контрольного вектора `123456789 -> 4B37`, реального
CMP1 event payload и инкрементального расчёта. `docs/17_UART_EVENT_TRANSPORT.md`
исправлен: рабочий протокол использует CRC-16/MODBUS (`0xFFFF`, reflected
`0xA001`), а не CRC-CCITT.

```text
CMP Protocol Tests: SUCCESS (run 31928080265)
Arduino Uno Build: SUCCESS (run 31928080266)
ESP32 Build: SUCCESS (run 31928080285)
```

Этот блок не меняет safety contract: удалённый job не включает SSR, физический
START остаётся локальным, автоматического resume после reboot нет.

## CRC подтверждений RUN — 2026-08-16

ESP32 формирует `ACK/NACK` с CRC-16/MODBUS, а Arduino проверяет checksum до
удаления RUN-события из очереди или изменения retry interval. Это закрывает
случай принятия повреждённого UART-подтверждения.

Новая Arduino временно принимает legacy `ACK/NACK` без CRC для staged rollout;
старый Arduino parser совместим с новым ESP32-кадром и игнорирует завершающее
поле. Любое другое число полей или неверный CRC отклоняются.

```text
a695440cbcae2582c158d1f29ff68cac5a38ba95
CMP Protocol Tests: SUCCESS (run 31929625664)
Arduino Uno Build: SUCCESS (run 31929625657)
ESP32 Build: SUCCESS (run 31929625636)
```
