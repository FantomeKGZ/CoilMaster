# CoilMaster — прикладной протокол поверх CMP

CMP отвечает за транспорт, кадрирование и CRC. Этот документ определяет прикладные сообщения CoilMaster.

## Общие правила

- каждое задание имеет `jobId`;
- повторная команда с тем же идентификатором не создает дубликат;
- многобайтовые числа используют Little Endian;
- Arduino является источником фактического результата;
- команды Web не включают SSR напрямую.

## Группы сообщений

### Синхронизация

- `HELLO`
- `CAPABILITIES`
- `TIME_SYNC`
- `STATE_REQUEST`
- `STATE_REPORT`

### Задания

- `JOB_OFFER`
- `JOB_ACCEPT`
- `JOB_REJECT`
- `JOB_READY`
- `JOB_START`
- `JOB_PAUSE`
- `JOB_RESUME`
- `JOB_CANCEL`
- `JOB_STATUS`

### Катушки

- `COIL_STARTED`
- `COIL_PROGRESS`
- `COIL_COMPLETED`
- `COIL_FAILED`
- `JOB_COMPLETED`
- `JOB_FAILED`

### Сервис

- `HALL_VALUE`
- `HALL_THRESHOLD_GET`
- `HALL_THRESHOLD_SET`
- `DIAG_REQUEST`
- `DIAG_RESULT`
- `BUZZER_TEST`
- `LCD_TEST`
- `SSR_TEST_PREPARE`
- `SSR_TEST_CONFIRM`

## Полезная нагрузка задания

```text
jobId:        uint32
source:       uint8
windingType:  uint8
coilCount:    uint8
turns[]:      uint16 × coilCount
```

На текущем этапе максимум катушек определяется возможностями Arduino Core и фиксируется общей константой.

## Подтверждение катушки

```text
jobId:          uint32
coilIndex:      uint8
requestedTurns: uint16
actualTurns:    uint16
result:         uint8
```

## Надежность

- важные команды требуют ACK;
- отправитель повторяет сообщение после тайм-аута ограниченное число раз;
- получатель хранит последний обработанный идентификатор команды;
- после восстановления связи выполняется обмен состояниями;
- неизвестный результат помечается `RESULT_UNKNOWN`, а не `COMPLETED`.
