# Журнал выполненных работ

Дата актуализации: 2026-08-08  
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

Это не заменяет обязательный production hardware E2E полного safety/workshop flow.

## 2. Архитектура безопасности

Зафиксировано и реализуется последовательно:

- Arduino отвечает за реальное время, SSR и физический цикл намотки;
- ESP32 отвечает за web, storage, workshop и доставку job;
- ESP32/WEB не включают SSR напрямую;
- физический START остаётся обязательным;
- автоматический resume после reboot запрещён;
- одновременно исполняется одна программа;
- `RUN_COMPLETED` сам по себе не списывает провод.

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

Для deep backup validation cursor pagination не используется: authoritative EOF validation находится в `CM_WindingJournalQueryValidation.cpp` и вызывается через `WindingJournalQuery::validateAll()`.

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

`/api/status` явно различает:

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

Последние UI commits этого этапа:

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

Реализованы и сохраняются:

- CU/AL/UNKNOWN;
- каталог диаметров;
- катушки и остатки;
- exact spool selection для linked winding;
- фильтры;
- material summary;
- ручной confirmed write-off;
- run-level provenance `source_session_id + source_run_id`;
- duplicate confirmed write-off guard для run;
- recoverable `PENDING → CONFIRMED | ABORTED` movement transaction;
- repair write-off history;
- server authoritative material totals;
- saved price/value/currency metadata.

Не начинать склад заново.

## 18. Дополнительные материалы и калькуляция ремонта

Реализованы:

- MaterialLedger;
- material usage transactions;
- crash recovery;
- repair reference validation;
- KGS currency policy;
- cost provenance;
- rounding/value integrity;
- repair costing;
- wire value split by CU/AL/UNKNOWN;
- pricing revision/history/audit UI;
- finalization preflight и CLOSED flow;
- archive/monthly reports.

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

Последняя документированная фактическая failure была syntax error в `CM_MaterialLedger.cpp`, исправленная commit `77fd7dd4db3767c33106d63e6f9174e6559b9bc8`. Последующие commits нельзя считать GREEN без фактического Actions result.

## 21. Read-only backup/export и deep persistence integrity

Реализован whitelist export:

```text
GET /api/backup/manifest
GET /api/backup/file
GET /api/backup/sessions
GET /api/backup/session-file
```

Heavy export/deep audit gated через `BackupActivityGuard::Safe`; при active winding deep scan не запускается.

Safe `snapshot_stable=true` требует integrity всего static whitelist и необходимых adjuncts:

- persistent allocator main/optional backup и отсутствие temp residue;
- conductor settings и отсутствие recovery residue;
- workshop clients/motors/repairs + repair-status;
- repair pricing и references;
- materials/usage/adjustments + formulas/references/recovery markers;
- winding journal schema до EOF + `RUN_STARTED → RUN_COMPLETED` transition audit;
- warehouse spools/price/movements;
- содержимое snapshot/state/spool-selection session files штатными parsers и cross-file identity.

`CM_WindingPersistenceIntegrityAudit` использует authoritative `WindingJournalQuery::validateAll()` и отдельно `WindingJournalTransitionAudit::validate()`. `CM_WindingSessionPersistenceIntegrityAudit` остаётся отдельным deep parser/cross-identity audit и не дублируется.

HTTP/error semantics read-only backup/run-level endpoints отдельно audited в `docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md`.

## 22. Stage 0 NDJSON/session observability

Решение зафиксировано в `docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md`: сначала измерять, не мигрировать преждевременно в БД и не вводить arbitrary rotation threshold.

Manifest возвращает 14 runtime metrics без отдельного telemetry full scan:

```text
snapshot_stability_duration_ms
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
workshop_client_record_count
workshop_motor_record_count
workshop_repair_record_count
repair_status_record_count
repair_pricing_record_count
winding_journal_record_count
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
warehouse_movement_record_count
```

Counts собираются внутри уже существующих authoritative validation passes. Старые `check(storage)` contracts сохранены через compatibility overloads. Partial metrics после failed domain audit не публикуются.

Последний business observability batch:

```text
cf7df132d190bd359a4f4b85b2553f6dcdba5dd4  Expose business data audit counts
33746bf301ee8a31417361ce6fd8f7a2ce1635f7  Count business backup audit records
1871d140e1e493b6e64ada3502eaa5fbcb75f0f6  Expose business audit counts in backup manifest
```

Параллельно сохранён winding session observability batch:

```text
9c33178d8b580460e1d34962322fe81b9771dccc  Expose winding session persistence counts
afd2c9e3df2e63b59553e4f10e12eb4d2199e46d  Count winding session persistence files
abc4b02ef284ed86fdfc3e31149ccf8adf9d5e8b  Expose winding session file counts in backup manifest
```

## 23. Текущая точка после выполненного

Основной production flow собран:

```text
client → motor → OPEN repair → costing → linked winding → exact spool
→ immutable snapshot/spool-selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual wire writeoff
→ costing → finalization preflight → CLOSED → reports → backup
```

Следующий обязательный внешний этап — реальный hardware E2E ESP32 + Arduino. Repository review не доказывает физический UART/START/SSR behavior.

На E2E/эксплуатационном стенде одновременно снять пары `size_bytes + record_count`, session file counts и `snapshot_stability_duration_ms`. Только после фактических измерений выбирать bounded in-request index, duplicate-audit decomposition или rotation. Database migration, persistent optimistic cache и arbitrary rotation threshold пока не вводить.

Если hardware временно недоступен, следующий repo-only код допустим только как same-pass observability существующего authoritative validator без дополнительного full scan либо как исправление доказанной correctness/compile проблемы.

Точная последовательность следующей работы находится в `06_ACTIVE_WORK_AND_NEXT_STEPS.md`; полный свежий snapshot — в `12_LATEST_HANDOFF_2026-08-08.md`.
