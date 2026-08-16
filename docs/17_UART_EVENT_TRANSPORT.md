# CoilMaster — текстовый UART-транспорт CMP1

## Подключение

```text
Arduino A1 (TX) -> LLC -> ESP32 GPIO16 (RX2)
Arduino A2 (RX) <- LLC <- ESP32 GPIO17 (TX2)
GND Arduino     -------- GND ESP32
```

Стороны логического преобразователя:

- Arduino Uno: HV = 5 V;
- ESP32: LV = 3.3 V;
- земля общая.

## Параметры линии

- скорость: `9600 baud`;
- формат: `8N1`;
- Arduino использует `SoftwareSerial` на A2/A1;
- ESP32 использует UART2 на GPIO16/GPIO17;
- USB Serial `115200` остаётся только для диагностики.

## Форматы сообщений

Каждое сообщение является одной ASCII-строкой, завершённой `\n`.

Кадры с CRC:

```text
CMP1|EVT|EVENT_NAME|SESSION_ID|RUN_ID|COMPLETED_RUNS|CRC16
CMP1|LOCAL_EVT|EVENT_NAME|SESSION_ID|RUN_ID|COMPLETED_RUNS|WINDING_TYPE|COIL_COUNT|TURNS|CRC16
CMP1|JOB|JOB_ID|SESSION_ID|WINDING_TYPE|COIL_COUNT|TURNS|C|CRC16
CMP1|JOB_CANCEL|JOB_ID|CRC16
CMP1|ACK|RUN_ID|STATUS|CRC16
CMP1|NACK|RUN_ID|REASON|CRC16
CMP1|JOB_ACK|JOB_ID|STATUS|DETAIL|C|CRC16
CMP1|JOB_CANCEL_ACK|JOB_ID|STATUS|DETAIL|C|CRC16
```

Legacy-ответы без CRC, принимаемые при поэтапном обновлении:

```text
CMP1|JOB_ACK|JOB_ID|STATUS|DETAIL
CMP1|JOB_CANCEL_ACK|JOB_ID|STATUS|DETAIL
```

ESP32 всегда добавляет CRC в `ACK/NACK`. Arduino проверяет CRC до обработки
подтверждения. На время поэтапного обновления новая Arduino также принимает
старый четырёхполевой `ACK/NACK` без CRC; кадр с дополнительным полем, которое
не является корректным CRC, отклоняется.

Поле `C` в новом `JOB` объявляет поддержку CRC ответов задания. Старая Arduino
игнорирует это поле и отвечает legacy-кадром, который новая ESP32 продолжает
принимать. Новая Arduino, получив `C`, возвращает `JOB_ACK/JOB_CANCEL_ACK` с
собственным полем `C` и CRC. Старый ESP32 не объявляет capability, поэтому новая
Arduino отвечает ему в старом формате. Таким образом, платы можно обновлять в
любом порядке. Оборванный negotiated-кадр с оставшимся `C`, но без CRC, не
принимается как legacy.

`EVENT_NAME` принимает `RUN_STARTED` или `RUN_COMPLETED`. `WINDING_TYPE` —
`WORKING` или `STARTING`. Значения витков нескольких катушек передаются через
запятую.

## CRC16

CRC рассчитывается по байтам строки до последнего разделителя `|` перед полем
CRC. Используется `CRC-16/MODBUS`:

- начальное значение: `0xFFFF`;
- отражённый полином: `0xA001` (обычная запись `0x8005`);
- вход и результат отражены;
- финальный XOR отсутствует;
- в строке результат записывается четырьмя шестнадцатеричными символами.

Контрольный вектор: строка `123456789` даёт `4B37`.

Общая реализация CRC для рабочего текстового транспорта находится в
`Shared/CMP1Text/CM_Cmp1Crc.h`. Код в `Shared/Protocol` относится к отдельному
экспериментальному бинарному формату и использует другой алгоритм CRC; смешивать
эти два формата нельзя.

## Поведение и безопасность

Arduino повторяет неподтверждённые события и обрабатывает `ACK`/`NACK`. ESP32
может передать задание или запрос отмены, но задание только ставится в очередь:
физический старт всегда требует локального подтверждения. ESP32 не получает
права напрямую управлять SSR.
