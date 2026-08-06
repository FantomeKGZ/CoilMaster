# Где остановились и что делать дальше

Дата обновления: 2026-08-06

## Что проверено в этой сессии

Прочитан весь каталог:

```text
docs/PROJECT_HANDOFF/
```

Проверены актуальные файлы ветки `cmp-protocol-v1`:

```text
firmware/esp32/src/CM_WindingJournal.h
firmware/esp32/src/CM_WindingJournal.cpp
firmware/esp32/src/CM_UartEventReceiver.h
.github/workflows/esp32-build.yml
.github/workflows/cmp-protocol-tests.yml
docs/79_MONOTONIC_WINDING_RUN_IDS_PER_SESSION.md
```

## Фактическое состояние предыдущих шагов

### Монотонный run_id

Уже реализовано:

- внутри одной `session_id` новый `RUN_STARTED` должен иметь `run_id`, строго больший всех ранее сохранённых запусков этой сессии;
- метод `loadSessionHighestRunId()` присутствует в заголовке и реализации;
- документ `docs/79_MONOTONIC_WINDING_RUN_IDS_PER_SESSION.md` существует.

### Составной ключ события

Код уже опережал прежний handoff-план.

Актуальная реализация использует:

```cpp
containsRunEvent(uint32_t sessionId,
                 uint32_t runId,
                 RemoteEventType type)
```

И поиск старта:

```cpp
hasRunStart(uint32_t sessionId, uint32_t runId)
```

Таким образом, идентичность события уже определяется как:

```text
session_id + run_id + event_type
```

Это документировано в:

```text
docs/80_COMPOSITE_WINDING_EVENT_IDENTITY.md
```

Коммит:

```text
7235f1e7e54e8603191685035e38eaeb333fef2e
```

## Статус CI

`NOT VERIFIED`.

Подключённый GitHub API не вернул workflow runs или status checks для проверенных прямых коммитов. Нельзя считать этап зелёным только по успешной записи файлов.

Нужно явно проверить на текущем head ветки:

```text
ESP32 Build
CMP Protocol Tests
```

Если один workflow красный, следующий кодовый этап не начинать до исправления фактической ошибки.

## Новое архитектурное решение

Создан документ:

```text
docs/81_WINDING_SESSION_ID_SEMANTICS.md
```

Коммит:

```text
0aa21ae9b3f6ee0fe5871eb500846d99732e882b
```

Зафиксирован контракт для следующей реализации:

- ESP32 является источником `session_id`;
- один `session_id` относится к одному неизменяемому снимку задания;
- Arduino не генерирует новый идентификатор, а повторяет принятый в событиях;
- повторы доставки одного задания сохраняют тот же идентификатор;
- новый снимок задания получает новый ненулевой идентификатор;
- идентификаторы не должны повторно использоваться для другого задания;
- состояние должно переживать перезапуск ESP32;
- отсутствие устойчивого хранилища блокирует выдачу нового производственного задания;
- наличие идентификатора не разрешает физический запуск и не управляет SSR.

## Текущая точка остановки

Следующий кодовый этап:

```text
Persistent session allocator + immutable job snapshot foundation
```

Перед реализацией нужно найти актуальное место, где:

- веб/API создаёт `CM::OutgoingWindingJob`;
- назначается `jobId`;
- назначается `sessionId`;
- вызывается `UartEventReceiver::queueJob()`;
- хранится состояние выбранного ремонта и двигателя.

Нельзя добавлять второй независимый генератор идентификаторов рядом с существующим кодом.

## Шаг 1 — обязательная проверка head

1. Проверить `ESP32 Build`.
2. Проверить `CMP Protocol Tests`.
3. Если есть ошибка, открыть конкретный job и исправить минимально.
4. Результат записать в `01_CURRENT_STATE.md`, `05_COMPLETED_WORK_LOG.md` и этот файл.

## Шаг 2 — найти текущий путь создания задания

Проверить все места использования:

```text
OutgoingWindingJob
queueJob(
jobId
sessionId
```

Составить фактическую карту:

```text
HTTP/UI request
→ validation
→ job snapshot creation
→ identifier allocation
→ queueJob()
→ JobDeliveryEvent
→ RemoteWindingEvent
→ journal
```

## Шаг 3 — спроектировать минимальное устойчивое хранилище

Минимальный снимок должен содержать:

```text
schema_version
job_id
session_id
repair_id
motor_id
program_type
coil_count
turns[]
wire_type
wire_diameter
created_at
created_uptime_ms
delivery_state
execution_state
```

Неизвестные значения должны быть представлены явно, а не подменяться нулём без семантики.

Рекомендуемые состояния первой версии:

```text
CREATED
DELIVERING
ACCEPTED
REJECTED
TIMED_OUT
CANCELLED
WAITING_PHYSICAL_START
RUNNING
PROGRAM_COMPLETED
FAULT
```

Перед фиксацией имён проверить существующие enum/строки состояния, чтобы не создать дубликаты.

## Шаг 4 — allocator session_id

Требования:

- ненулевой `uint32_t`;
- монотонное увеличение;
- запись нового значения до доставки задания;
- восстановление после перезапуска;
- fallback к максимальному валидному `session_id` из снимков;
- отказ при переполнении;
- отказ при недоступной или противоречивой SD;
- отсутствие автоматического запуска оборудования.

## Шаг 5 — связать job и события

После появления снимка расширить протокол согласованно для Arduino и ESP32, чтобы `RUN_STARTED/RUN_COMPLETED` можно было однозначно связать с `job_id` и конкретной катушкой.

Изменение формата выполнять одновременно для:

- Arduino;
- ESP32;
- тестов;
- документации;
- правил обратной совместимости.

## Шаг 6 — отказоустойчивость

После реализации allocator/snapshot проверить:

1. ESP32 перезапустилась до `JOB_ACK`.
2. Arduino приняла задание, но ACK потерян.
3. ESP32 перезапустилась после `RUN_STARTED`.
4. Arduino перезапустилась во время намотки.
5. SD недоступна при выделении `session_id`.
6. SD заполнилась при обновлении снимка.
7. повторно пришёл `RUN_COMPLETED`.
8. пришёл старый ACK от предыдущего задания.
9. связь восстановилась после таймаута.

Во всех случаях SSR остаётся под контролем Arduino и переходит в безопасное состояние.

## Ближайший короткий план

```text
1. Подтвердить GREEN для ESP32 Build и CMP Protocol Tests.
2. Найти все места создания OutgoingWindingJob и вызова queueJob().
3. Не создавать дублирующий генератор job/session ID.
4. Спроектировать минимальный JobSnapshotStore.
5. Реализовать устойчивый allocator session_id небольшим отдельным этапом.
6. Добавить тесты восстановления и отказа SD.
7. Обновить тематический документ и handoff-файлы.
```
