# CoilMaster — прикладной UART/CMP1 протокол

Этот документ описывает фактический application-layer протокол текущей ветки `cmp-protocol-v1` между ESP32 и Arduino Uno.

## Общие правила

- текстовые кадры начинаются с `CMP1`;
- поля разделяются символом `|`;
- критические кадры защищаются CRC16 в последнем поле;
- каждое remote-задание имеет уникальный ненулевой `job_id`;
- физический результат намотки формирует Arduino;
- ESP32/Web не управляют SSR и не создают physical START;
- retries ограничены и не должны создавать дубликаты операций.

## Remote JOB

ESP32 передает Arduino задание формата семейства:

```text
CMP1|JOB|<job_id>|<session_id>|<type>|<coil_count>|<turns_csv>|...|<CRC>
```

Точная сериализация определяется текущей реализацией `CM_UartEventReceiver` / Arduino transport. Семантически обязательны:

- `job_id`;
- `session_id`;
- тип `WORKING` или `STARTING`;
- число катушек;
- программа витков.

Arduino отвечает:

```text
CMP1|JOB_ACK|<job_id>|ACCEPTED|<detail>|C|<CRC>
CMP1|JOB_ACK|<job_id>|REJECTED|<detail>|C|<CRC>
```

Поле `C` означает поддержку CRC-защищенного reply capability в текущем handshake.

## Retry доставки

ESP32 повторяет `JOB` после retry interval ограниченное количество раз. Потерянный ACK не означает, что Arduino не получила задание.

Если оператор отменяет задание после хотя бы одной возможной передачи, ESP32 обязана прекратить JOB retransmit и перейти к remote `JOB_CANCEL`, а не объявлять задание локально отмененным.

## JOB_CANCEL

ESP32 отправляет отмену конкретного no-run задания:

```text
CMP1|JOB_CANCEL|<job_id>|<CRC>
```

Arduino отвечает:

```text
CMP1|JOB_CANCEL_ACK|<job_id>|CANCELLED|<detail>|C|<CRC>
CMP1|JOB_CANCEL_ACK|<job_id>|REJECTED|<detail>|C|<CRC>
```

Семантика отмены:

- exact remote job в `Ready` без run evidence может быть очищен;
- если remote job уже отсутствует, повторная отмена должна завершаться идемпотентно, а не оставлять стороны в бесконечном reject/retry;
- другой активный/busy remote job не должен быть очищен чужим `job_id`;
- задание с физическим run evidence отменять этим recovery path запрещено.

## Физический ALL_CLEAR fallback

На Arduino предусмотрена операторская комбинация:

```text
D → * → # → D
```

Она используется только для восстановления рассинхронизации no-run remote job.

При безопасном clear Arduino отправляет:

```text
CMP1|JOB_CANCEL_ACK|0|CANCELLED|ALL_CLEAR|C|<CRC>
```

Особенности:

- `job_id=0` является специальным физическим clear-сигналом, а не обычной отменой job №0;
- ESP32 принимает его только с валидным CRC и detail `ALL_CLEAR`;
- ESP32 коррелирует его с текущим pending/recovered job ID;
- результат проходит через обычный persisted cancellation path;
- `ALL_CLEAR` никогда не преобразуется в `RUN_COMPLETED`.

## Фактические run events

Arduino передает события вида:

```text
CMP1|EVT|RUN_STARTED|<session_id>|<run_id>|0|<CRC>
CMP1|EVT|RUN_COMPLETED|<session_id>|<run_id>|<completed_runs>|<CRC>
```

Для локальных standalone программ Arduino может передавать `LOCAL_EVT` с дополнительными полями программы.

ESP32 подтверждает run event через CRC-защищенный ACK/NACK reply.

## Safety semantics

- `JOB_ACK ACCEPTED` означает только принятие задания Arduino, не запуск двигателя;
- physical START выполняется только физическим вводом на Arduino;
- `RUN_STARTED` является фактом начала конкретного run;
- `RUN_COMPLETED` является фактом завершения run, но не списанием провода;
- manual wire writeoff требует exact `spool_id + source_session_id + source_run_id`;
- reboot не должен автоматически возобновлять намотку или unfinished cancel/writeoff action.

## Надежность и восстановление

- duplicate/retry handling должно быть идемпотентным;
- неизвестность после потери ACK сохраняется как uncertainty, а не подменяется положительным результатом;
- persisted active/recovered job ID должен быть доступен UART receiver после reboot для корреляции физического `ALL_CLEAR`;
- no-run cancellation может закрыть delivery/waiting-physical-start состояния только при отсутствии run evidence;
- protocol errors должны fail-safe не влиять на SSR.
