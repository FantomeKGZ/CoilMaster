# CoilMaster — winding session / run workflow

## 1. Назначение

Документ фиксирует текущую production-связь между ESP32, Arduino Uno, Web, winding persistence и manual material writeoff.

Основная safety boundary:

```text
ESP32/Web подготавливают и доставляют JOB
Arduino принимает JOB
физический/local START запускает движение
```

Прием JOB, ACK, recovery или Web-действие сами по себе **никогда не являются physical START**.

## 2. Основные сущности

### WindingProgram / WindingJob

Программа содержит последовательность катушек/целевых витков. Для remote linked production ESP32 формирует job с устойчивыми `job_id` / `session_id`, immutable snapshot и exact material selection до UART boundary.

Сам факт существования или доставки job не означает, что намотка началась.

### WindingSession

`session_id` объединяет job/session-level immutable evidence и несколько фактических запусков одной программы там, где оператор повторяет ее вручную.

Для linked production current source of truth также содержит immutable exact spool selection, относящийся к этому session.

### WindingRun

Каждое фактическое выполнение программы имеет отдельный `run_id`.

```text
RUN_STARTED(session_id, run_id, ...)
RUN_COMPLETED(session_id, run_id, ...)
```

`run_id` сохраняется между STARTED и COMPLETED одного прохода. Повтор программы создает новый run; исторические run records не сливаются и не удаляются только ради UI-группировки.

## 3. Current production transport

RUN events уже передаются между Arduino и ESP32 через production text protocol `CMP1|...`; это не будущий этап.

Owners:

```text
Arduino/CM_UartEventTransport.h/.cpp
firmware/esp32/src/CM_UartEventReceiver.h/.cpp
Shared/CMP1Text/CM_Cmp1Crc.h
```

`Shared/Protocol/` — отдельный старый binary host-test protocol и не заменяет production CMP1.

Arduino transport имеет bounded queue/retry/ACK-NACK handling. Повторная доставка события не должна создавать второй фактический run record на ESP32.

Lost ACK / timeout не доказывает, что Arduino idle. Recovery/timeout states не дают права автоматически запускать следующую работу.

## 4. Linked production preparation до UART

Текущий high-level flow:

```text
client
-> motor
-> OPEN repair
-> costing
-> linked winding
-> persistent job/session identity
-> immutable job snapshot
-> exact immutable spool selection
-> UART JOB delivery
```

Перед переходом linked job к delivery должны существовать согласованные persistence evidence. Current exact spool selection содержит как минимум:

```text
job_id
session_id
repair_id
motor_id
spool_id
diameter
wire_type
weight_at_selection
```

Historical `UNALLOCATED` movement evidence не отменяет этот current pre-UART requirement.

## 5. Прием remote JOB на Arduino

Arduino валидирует и принимает remote job через CMP1 transport/state ownership. Accepted/ACK означает только, что job принят/сохранен в допустимое локальное состояние.

После приема Arduino показывает/удерживает программу и ждет **local physical operator input** для START.

Запрещено трактовать как START:

- Web POST/GET;
- UART JOB frame;
- ACK/NACK;
- reconnect;
- timeout recovery;
- reboot;
- repeat counter;
- получение следующего job.

## 6. Physical run

При разрешенном local physical START:

1. Arduino создает/активирует новый exact `run_id`.
2. Формируется `RUN_STARTED`.
3. Arduino realtime state machine владеет SSR и Hall counting.
4. Между этапами/катушками сохраняется требуемая локальная operator confirmation boundary.
5. После фактического завершения прохода формируется `RUN_COMPLETED` с тем же `run_id`.
6. Event transport доставляет evidence ESP32 с bounded retry/ACK behavior.
7. ESP32 сохраняет immutable winding journal evidence.

ESP32/Web никогда напрямую не управляют SSR.

## 7. Repeat semantics

Повтор полного winding program всегда является новым физическим run и получает новый `run_id`.

Никакой repeat count не разрешает automatic physical START. Между полными проходами требуется новое local operator action.

Final repeat не может автоматически reopen job/session для еще одного движения.

UI может показывать агрегированное `выполнено N раз`, но authoritative evidence остается run-level.

## 8. Reboot / communication recovery

После reboot нет automatic winding resume.

При потере ACK или связи:

- уже начатый физический процесс и его safety state принадлежат Arduino;
- ESP32 не делает вывод `Arduino idle` только по timeout;
- late valid `RUN_STARTED` / `RUN_COMPLETED` обрабатываются по recovery/state rules;
- ambiguous state требует fail-closed/manual review, а не автоматического нового START.

Recovery не должна стирать immutable run/history evidence.

## 9. RUN_COMPLETED и material writeoff — разные границы

`RUN_COMPLETED` означает факт завершения физического run. Он **не** списывает провод.

После сохранения run evidence оператор выполняет отдельный explicit/manual writeoff. Для current linked production обязательна provenance:

```text
source_session_id + source_run_id + exact immutable spool_id
```

Writeoff owner повторно проверяет exact run completion и immutable spool selection. Один completed run не может быть использован для двух confirmed writeoff.

Historical `UNALLOCATED` KG_FIRST records остаются compatibility evidence только для read/audit/recovery. Они не создают post-run optional-spool fallback.

## 10. Repair finalization

Repair closure/finalization проверяет run/material coverage на уровне exact persisted evidence. Нельзя закрывать linked production только по неоднозначному session-level признаку, если конкретный completed run требует exact writeoff coverage.

Production tail:

```text
RUN_COMPLETED
-> explicit manual exact-run exact-spool writeoff
-> costing / persisted pricing evidence
-> finalization preflight
-> CLOSED
-> reports
-> backup
```

## 11. Local program behavior

Локальная программа Arduino может существовать без linked repair/motor workflow и отправлять свои фактические run events ESP32 через текущий CMP1 transport.

Ее наличие не ослабляет linked-production rules: если операция относится к linked production repair/job, authoritative linkage, immutable selection и exact-run writeoff requirements должны быть выполнены соответствующим production flow.

## 12. Web/UI rules

Web может:

- подготовить linked job;
- выбрать exact spool до delivery;
- показать state/run history;
- инициировать безопасные data/API операции;
- предложить оператору manual writeoff/finalization actions.

Web не может:

- физически запустить станок;
- напрямую включить SSR;
- считать timeout доказательством idle;
- автоматически списать wire из `RUN_COMPLETED`;
- автоматически возобновить winding после reboot;
- скрыть выбранный exact spool при последующем linked writeoff.

## 13. Authoritative owners / verification

```text
Core/CM_StateMachine.*
Core/CM_WindingEvent.h
Core/CM_WindingJob.h
firmware/arduino/src/main.cpp
Arduino/CM_UartEventTransport.*
firmware/esp32/src/CM_UartEventReceiver.*
firmware/esp32/src/CM_JobStateStore.*
firmware/esp32/src/CM_JobSnapshotStore.*
firmware/esp32/src/CM_JobSpoolSelectionStore.*
firmware/esp32/src/CM_WindingJournal*
firmware/esp32/src/CM_WarehouseWriteOff*
```

Changes touching this workflow require applicable CMP Protocol/Arduino/ESP32/Web regression gates. Hardware behavior is a separate physical acceptance gate and must never be inferred from CI alone.
