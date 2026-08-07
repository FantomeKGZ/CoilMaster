# Журнал выполненных работ

Дата актуализации: 2026-08-07  
Ветка: `cmp-protocol-v1`

Этот файл — укрупнённая карта уже выполненной работы. Для точного diff и полного списка промежуточных коммитов использовать историю GitHub и тематические документы в `docs/`.

## 1. Аппаратная часть и базовый обмен

Проверены пользователем:

- основные модули ESP32;
- DS3231;
- microSD;
- Arduino Uno и связанные модули;
- level shifter;
- UART ESP32 ↔ Arduino.

Статус аппаратной проверки и базового обмена: `USER CONFIRMED`.

## 2. Архитектура безопасности

Зафиксировано и реализуется последовательно:

- Arduino отвечает за реальное время, SSR и физический цикл намотки;
- ESP32 отвечает за web, storage, workshop и доставку job;
- ESP32/WEB не включают SSR напрямую;
- физический START остаётся обязательным;
- автоматический resume после reboot запрещён;
- одновременно исполняется одна программа.

## 3. CMP/UART доставка задания

Реализованы:

- строковый протокол `CMP1|...`;
- строгий parser полей;
- bounded retry;
- ACK/REJECT/TIMEOUT/CANCEL;
- защита непрочитанного delivery result;
- pending-job cancellation;
- строгая job identity;
- приём `RUN_STARTED` и `RUN_COMPLETED`.

## 4. Persistent job identity

Реализован устойчивый allocator:

```text
firmware/esp32/src/CM_PersistentIdAllocator.h
firmware/esp32/src/CM_PersistentIdAllocator.cpp
```

Хранилище:

```text
/data/winding-jobs/id-state.txt
```

`job_id` и `session_id` больше не являются только RAM-счётчиками и переживают reboot.

## 5. Immutable snapshot задания

Реализованы:

```text
CM_JobSnapshotStore.h/.cpp
CM_JobDisplayRecovery.h/.cpp
```

Snapshot хранит исходную identity/program/linkage задания и после создания не перезаписывается.

Путь:

```text
/data/winding-jobs/snapshots/session-<session_id>.json
```

Parser snapshot усилен fail-closed: canonical numbers, duplicate-key rejection, строгий nullable linkage, program type и turns array.

## 6. Persistent runtime-state и recovery

Реализованы:

```text
CM_JobStateStore.h/.cpp
CM_JobRecovery.h/.cpp
```

Состояния доставки и выполнения сохраняются отдельно от snapshot.

Путь:

```text
/data/winding-jobs/state/session-<session_id>.json
```

Реализованы:

- atomic temp/write/verify/rename;
- strict state transitions;
- `loadLatest()`;
- fail-closed на malformed state-файле;
- snapshot identity cross-check;
- manual review для опасных recovery-состояний;
- `CLOSED_AFTER_REVIEW`;
- `POST /api/recovery/acknowledge`;
- `automatic_queue_allowed=false`;
- `automatic_resume_allowed=false`.

## 7. Linked repair/motor job

Реализованы:

```text
CM_JobLinkageRequest.h/.cpp
CM_JobLinkageResolver.h/.cpp
```

`POST /api/jobs` поддерживает строго связанный режим:

```text
repair_id + motor_id
```

Перед выделением job/session ID сервер:

- проверяет оба ID;
- проверяет repair → motor;
- находит motor в каталоге;
- получает authoritative `coil_program`;
- сравнивает submitted turns с программой двигателя;
- fail-closed при ambiguity/corruption.

## 8. Общий parser программы намотки

Создан:

```text
firmware/esp32/src/CM_WindingProgramParser.h
```

Используется job creation, RepairRegistry, similarity и web UI.

Правила:

- 1..10 сегментов;
- 1..9999 витков;
- `/`, `,`, `;` как разделители;
- пробелы игнорируются;
- leading zero и пустые сегменты запрещены;
- canonical representation `N/N/...`.

## 9. Реестр клиентов, двигателей и ремонтов

Реализованы:

```text
CM_RepairRegistry.h/.cpp
CM_RepairRegistryWeb.h/.cpp
CM_RepairRegistrySimilarity.cpp
CM_MotorSimilarityWeb.h/.cpp
```

API:

```text
GET/POST /api/clients
GET/POST /api/motors
GET/POST /api/repairs
GET      /api/motors/similar
```

Хранилище:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
```

Integrity guards:

- unique canonical IDs;
- repair references to exactly one client/motor;
- valid stored `coil_program`;
- canonical new `coil_program`;
- duplicate string-field rejection;
- partial-write fail-closed;
- runtime storage readiness.

## 10. Similarity двигателей

Реализовано предупреждение о похожих карточках.

Сравнение `coil_program` теперь семантическое, поэтому:

```text
100/200
100,200
100 ; 200
```

трактуются как одна программа после parsing, а не как разные строки.

Это не считается полным детектором одинаковой обмотки/аналога двигателя.

## 11. Winding journal schema 2

Журнал:

```text
/data/winding-runs/events.ndjson
```

Schema 2 содержит:

```text
job_id
session_id
run_id
event
repair_id/motor_id или null
completed_runs
uptime_ms
```

Реализованы:

- composite event identity;
- deduplication;
- start-before-complete;
- one active run per session;
- monotonic run_id;
- sequential completed_runs;
- immutable session context;
- strict canonical numeric/null parser;
- duplicate-key rejection;
- единственный `event`;
- fail-closed scan при corruption.

Последние writer-side commits:

```text
b33ff222617cce7ed1fd41a069a5bdeb8ff323d8
a3b59cff66d48538cc38087c65c968346c86f54a
```

## 12. Read-only winding history

Реализованы:

```text
CM_WindingJournalQuery.h/.cpp
CM_WindingJournalWeb.h/.cpp
```

API:

```text
GET /api/winding-history
```

Фильтр — ровно один из:

```text
session_id
repair_id
```

Есть cursor pagination, `next_cursor`, `has_more` и fail-closed чтение corrupted schema 2.

Legacy schema 1 проверяется структурно, но не отдаётся новым API, потому что не содержит immutable repair/motor context.

UI:

```text
mobile/winding-history.html
desktop/winding-history.html
```

## 13. Runtime microSD readiness

После boot критические компоненты больше не полагаются только на cached-ready флаг.

Динамически проверяются:

- winding journal;
- ID allocator;
- snapshot store;
- state store;
- winding history;
- static site storage;
- RepairRegistry;
- JobLinkageResolver.

При потере карты новое job блокируется до выделения ID/отправки UART.

## 14. Lifecycle API и главные страницы

`/api/status` теперь явно различает:

```text
WAITING_ARDUINO_ACK
ACCEPTED_READY
RUNNING
PROGRAM_COMPLETED
REJECTED
TIMED_OUT
CANCELLED
MANUAL_REVIEW_REQUIRED
```

Исправлено отображение завершённого job: после `RUN_COMPLETED` UI показывает завершение, а не старое `ACCEPTED_READY`.

Обычная занятость текущим job больше не выдаётся как `manual_review_required`; используется `current_job_not_complete`.

Mobile/desktop index обновлены под эти состояния.

Последние UI commits:

```text
4e56ac07590be3eb44665b7a4f1f1c0fa39d5423
0703f828163513007e2694af28467a684d0700b2
```

## 15. Linked winding UI

Созданы mobile и desktop страницы связанной намотки:

```text
mobile/winding-job.html
desktop/winding-job.html
```

Они:

- получают `repair_id`;
- загружают repair и motor;
- показывают motor `coil_program` readonly;
- отправляют repair_id + motor_id в `/api/jobs`;
- не выполняют физический старт.

## 16. Motor catalogue UI

Mobile и desktop `motors.html`:

- создают карточки двигателя;
- ищут по каталогу;
- вызывают similarity API;
- предварительно валидируют программу;
- канонизируют её в `N/N/...` до API.

Коммиты последней UI-валидации:

```text
90841c8dc43fde2511a5180d528829ca0cc46d55
b57a898d3921ef4c0c7dbf4a17a8e32770abbe4a
```

## 17. Склад провода

Ранее реализованы и сохраняются:

- CU/AL/UNKNOWN;
- каталог диаметров;
- катушки и остатки;
- фильтры;
- material summary;
- confirmed write-off;
- legacy material assignment;
- repair write-off history;
- server authoritative material totals;
- saved price/value/currency metadata.

Не начинать склад заново.

## 18. Дополнительные материалы и калькуляция ремонта

Ранее реализованы:

- MaterialLedger;
- material usage transactions;
- crash recovery;
- repair reference validation;
- KGS currency policy;
- cost provenance;
- rounding/value integrity;
- repair costing;
- wire value split by CU/AL/UNKNOWN;
- pricing revision/history/audit UI.

Не начинать эти подсистемы заново.

## 19. Conductor calculator

Реализованы:

- bidirectional Al ↔ Cu calculator core;
- server-authoritative settings;
- warehouse-aware catalogue/recommendations;
- mobile/desktop calculator UI.

Расширенный генератор/справочник может развиваться отдельно, но существующий calculator не является пустым местом.

## 20. CI

В ветке присутствуют:

```text
.github/workflows/esp32-build.yml
.github/workflows/arduino-uno-build.yml
.github/workflows/cmp-protocol-tests.yml
```

Пользователь 2026-08-07 подтвердил зелёными проверенные им предыдущие коммиты. Документация не должна использоваться как доказательство GREEN для последующих commits; при наличии доступа сверять фактический Actions run.

## 21. Текущая точка после выполненного

Инфраструктура full winding flow в основном собрана. Следующий участок — не новый storage/parser, а пользовательская цепочка ремонта:

```text
client
→ motor
→ repair
→ linked winding
→ physical run
→ winding history
```

Начать с актуальных:

```text
firmware/esp32/web/mobile/repairs.html
firmware/esp32/web/desktop/repairs.html
```

Точная последовательность следующей работы находится в `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
