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

Schema 2 содержит `job_id`, `session_id`, `run_id`, `event`, linkage, `completed_runs`, `uptime_ms`.

Реализованы composite identity, deduplication, start-before-complete, one active run per session, monotonic `run_id`, sequential `completed_runs`, immutable session context, strict canonical parser и fail-closed corruption scan.

## 12. Read-only winding history

API:

```text
GET /api/winding-history
```

Cursor pagination используется только пользовательским history API. Deep backup использует authoritative EOF validation из `CM_WindingJournalQueryValidation.cpp` через `WindingJournalQuery::validateAll()`.

## 13. Runtime microSD readiness

Критические stores динамически fail-closed при потере microSD. При потере карты новое job блокируется до выделения ID/отправки UART.

## 14. Lifecycle API и UI

`/api/status` различает waiting/accepted/running/completed/rejected/timeout/cancel/manual-review states. Mobile/desktop UI синхронизированы с lifecycle и не выполняют physical START.

## 15. Linked winding и exact spool

Linked winding UI получает repair, authoritative motor `coil_program` и exact active CU/AL spool. Immutable spool selection сохраняется до UART delivery.

## 16. Motor catalogue

Motor catalogue/similarity реализованы; similarity не считается полным доказательством одинаковой обмотки.

## 17. Склад провода

Реализованы CU/AL/UNKNOWN, exact spool selection, ручной confirmed writeoff, run-level provenance `source_session_id + source_run_id`, duplicate guard, recoverable `PENDING → CONFIRMED | ABORTED`, history и server-authoritative totals.

## 18. Материалы, costing, finalization

Реализованы MaterialLedger, recovery, KGS policy, historical cost provenance, repair costing, pricing history, finalization preflight, CLOSED flow, archive/monthly reports.

## 19. Conductor calculator

Существующий bidirectional Al ↔ Cu calculator, server-authoritative settings и warehouse-aware recommendations реализованы. Расширенный генератор не начинать как замену существующего calculator без отдельной инженерной спецификации.

## 20. CI

Workflows присутствуют:

```text
.github/workflows/esp32-build.yml
.github/workflows/arduino-uno-build.yml
.github/workflows/cmp-protocol-tests.yml
```

Последняя документированная фактическая failure была syntax error в `CM_MaterialLedger.cpp`, исправленная commit `77fd7dd4db3767c33106d63e6f9174e6559b9bc8`. Последующие commits нельзя считать GREEN без фактического Actions result.

## 21. Read-only backup/export и deep persistence integrity

Реализованы:

```text
GET /api/backup/manifest
GET /api/backup/file
GET /api/backup/sessions
GET /api/backup/session-file
```

Heavy export/deep audit gated через `BackupActivityGuard::Safe`.

Safe `snapshot_stable=true` требует integrity allocator/settings, workshop/pricing, materials, winding journal + transitions, warehouse persistence/movements и содержимого snapshot/state/spool-selection session files с cross-file identity.

`CM_WindingPersistenceIntegrityAudit` использует `WindingJournalQuery::validateAll()` + отдельный transition audit. `CM_WindingSessionPersistenceIntegrityAudit` остаётся authoritative deep parser/cross-identity audit.

## 22. Stage 0 observability — 29 metrics

Strategy: `docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md`.

Manifest возвращает 29 metrics без telemetry-only full scan.

Per-domain timing:

```text
snapshot_stability_duration_ms
persistent_id_audit_duration_ms
conductor_settings_audit_duration_ms
material_persistence_audit_duration_ms
business_data_audit_duration_ms
winding_persistence_audit_duration_ms
warehouse_persistence_audit_duration_ms
warehouse_movements_audit_duration_ms
winding_session_directory_scan_duration_ms
winding_session_persistence_audit_duration_ms
```

Population/high-water/bytes:

```text
winding_allocator_last_id
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
winding_snapshot_total_bytes
winding_state_total_bytes
winding_spool_selection_total_bytes
warehouse_spool_record_count
warehouse_price_record_count
warehouse_movement_record_count
```

Timing измеряет только уже существующие audit calls через `millis()`; дополнительного SD I/O нет. Длительность failed domain публикуется, неисполненные последующие domains остаются `null`.

Allocator high-water, counts и byte totals также собираются в существующих authoritative passes. Session byte overflow является telemetry-only и не меняет integrity result.

Последние commits этого расширения:

```text
4a30e4ca08e1d2e010dded1ab3e93073f9ecaeed  Expose persistent allocator audit metrics
b38bb3b5190bb99d261f5552cecedfea4048289b  Return validated allocator high-water mark
52fae7716034ccacebc41f1f11715f5eebf193c2  Expose allocator high-water mark in backup manifest
1470b866c0b91aee4bd8dff1eddc6c26926be578  Expose winding session byte totals
cacdffa9ec822ad1425d6a4de34c10f836fbbab0  Measure winding session persistence bytes
a0c83b08f64c05f0232d287146850f9e9fd37ce5  Expose winding session byte totals in backup manifest
96a1c5bc8c4a5cb7f5b672d290bbac23867429c5  Measure deep backup domain durations
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

На стенде сохранить один manifest с `items[].size_bytes` и всеми 29 Stage 0 metrics. Сначала выбрать самый дорогой `*_duration_ms`, затем сопоставить его с counts/bytes/high-water. Только после измерений выбирать bounded index, duplicate-audit decomposition или rotation.

Database migration, persistent optimistic cache и arbitrary rotation threshold пока не вводить.

Если hardware временно недоступен, repo-only код допустим только как same-pass observability без дополнительного full scan либо как fix доказанной correctness/compile проблемы.

Точная последовательность — `06_ACTIVE_WORK_AND_NEXT_STEPS.md`; полный свежий snapshot — `12_LATEST_HANDOFF_2026-08-08.md`.
