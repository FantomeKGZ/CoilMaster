# Индекс ключевых файлов

Перед редактированием всегда получать актуальное содержимое файла из ветки `cmp-protocol-v1`.

## Главная интеграция ESP32

```text
firmware/esp32/src/main.cpp
```

Содержит:

- `/api/status`;
- `/api/jobs`;
- `/api/recovery/acknowledge`;
- регистрацию workshop/warehouse/static web API;
- startup всех persistent-компонентов;
- lifecycle текущего job;
- `job_creation_ready` и `linked_job_creation_ready`.

## Persistent job identity и recovery

```text
firmware/esp32/src/CM_PersistentIdAllocator.h
firmware/esp32/src/CM_PersistentIdAllocator.cpp
firmware/esp32/src/CM_JobSnapshotStore.h
firmware/esp32/src/CM_JobSnapshotStore.cpp
firmware/esp32/src/CM_JobStateStore.h
firmware/esp32/src/CM_JobStateStore.cpp
firmware/esp32/src/CM_JobRecovery.h
firmware/esp32/src/CM_JobRecovery.cpp
firmware/esp32/src/CM_JobDisplayRecovery.h
firmware/esp32/src/CM_JobDisplayRecovery.cpp
```

Хранилища:

```text
/data/winding-jobs/id-state.txt
/data/winding-jobs/snapshots/session-<session_id>.json
/data/winding-jobs/state/session-<session_id>.json
```

## UART и журнал намотки

```text
firmware/esp32/src/CM_UartEventReceiver.h
firmware/esp32/src/CM_UartEventReceiver.cpp
firmware/esp32/src/CM_WindingJournal.h
firmware/esp32/src/CM_WindingJournal.cpp
firmware/esp32/src/CM_WindingJournalSnapshotContext.cpp
```

Назначение:

- доставка job и ACK/REJECT/TIMEOUT/CANCEL;
- `RUN_STARTED` / `RUN_COMPLETED`;
- composite identity `session_id + run_id + event_type`;
- schema 2 context `job_id/repair_id/motor_id`;
- fail-closed event scans и startup validation.

Журнал:

```text
/data/winding-runs/events.ndjson
```

## История и full validation журнала намотки

```text
firmware/esp32/src/CM_WindingJournalQuery.h
firmware/esp32/src/CM_WindingJournalQuery.cpp
firmware/esp32/src/CM_WindingJournalQueryValidation.cpp
firmware/esp32/src/CM_WindingJournalWeb.h
firmware/esp32/src/CM_WindingJournalWeb.cpp
```

`CM_WindingJournalQueryValidation.cpp` содержит authoritative `WindingJournalQuery::validateAll()` / count overload для full-file schema scan до EOF. Не искать эту реализацию только в `CM_WindingJournalQuery.cpp` и не возвращать backup audit к cursor-pagination.

API:

```text
GET /api/winding-history?repair_id=<id>&cursor=<n>&limit=<n>
GET /api/winding-history?session_id=<id>&cursor=<n>&limit=<n>
```

UI:

```text
firmware/esp32/web/mobile/winding-history.html
firmware/esp32/web/desktop/winding-history.html
```

## Программа намотки

Единый parser:

```text
firmware/esp32/src/CM_WindingProgramParser.h
```

Правила:

- 1..10 катушек;
- 1..9999 витков на сегмент;
- разделители `/`, `,`, `;`;
- пробелы игнорируются;
- leading zero и пустые сегменты запрещены;
- canonical format `N/N/...`.

Используется job creation, registry, similarity и UI-валидацией.

## Linked job и workshop registry

```text
firmware/esp32/src/CM_JobLinkageRequest.h
firmware/esp32/src/CM_JobLinkageRequest.cpp
firmware/esp32/src/CM_JobLinkageResolver.h
firmware/esp32/src/CM_JobLinkageResolver.cpp
firmware/esp32/src/CM_RepairRegistry.h
firmware/esp32/src/CM_RepairRegistry.cpp
firmware/esp32/src/CM_RepairRegistrySimilarity.cpp
firmware/esp32/src/CM_RepairRegistryWeb.h
firmware/esp32/src/CM_RepairRegistryWeb.cpp
firmware/esp32/src/CM_MotorSimilarityWeb.h
firmware/esp32/src/CM_MotorSimilarityWeb.cpp
```

Данные:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
```

API:

```text
GET/POST /api/clients
GET/POST /api/motors
GET/POST /api/repairs
GET      /api/motors/similar
```

## Linked winding UI

```text
firmware/esp32/web/mobile/winding-job.html
firmware/esp32/web/desktop/winding-job.html
```

Ожидается `repair_id`, загружается repair + motor, `coil_program` readonly, POST идёт в `/api/jobs` с `repair_id` и `motor_id`. Физический запуск не выполняется из браузера.

## Главные страницы

```text
firmware/esp32/web/mobile/index.html
firmware/esp32/web/desktop/index.html
```

Показывают явные lifecycle states и readiness. Linked repair ведёт на winding history.

## Клиенты, ремонты и двигатели UI

```text
firmware/esp32/web/mobile/clients.html
firmware/esp32/web/mobile/repairs.html
firmware/esp32/web/mobile/motors.html
firmware/esp32/web/mobile/more.html
firmware/esp32/web/desktop/clients.html
firmware/esp32/web/desktop/repairs.html
firmware/esp32/web/desktop/motors.html
```

Основной production UI flow уже собран. Текущий обязательный следующий внешний этап — hardware E2E ESP32 + Arduino, а не повторная реализация repair/winding UI.

## Static web storage

```text
firmware/esp32/src/CM_StaticSiteServer.h
firmware/esp32/src/CM_StaticSiteServer.cpp
firmware/esp32/web/mobile/
firmware/esp32/web/desktop/
```

`web_storage_ready` динамически отражает runtime-доступность `/web`.

## Склад провода

Основные файлы:

```text
firmware/esp32/src/CM_WarehouseStore.h
firmware/esp32/src/CM_WarehouseStore.cpp
firmware/esp32/src/CM_WarehouseWeb.h
firmware/esp32/src/CM_WarehouseWeb.cpp
firmware/esp32/src/CM_WarehouseWriteOff.cpp
firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp
firmware/esp32/src/CM_WarehouseWriteOffHistory.cpp
firmware/esp32/src/CM_WarehouseMaterialCatalogue.cpp
```

Не начинать склад заново — подсистема уже реализована.

## Калькуляция ремонта и материалы

```text
firmware/esp32/src/CM_RepairCosting.h
firmware/esp32/src/CM_RepairCosting.cpp
firmware/esp32/src/CM_RepairCostingWeb.h
firmware/esp32/src/CM_RepairCostingWeb.cpp
firmware/esp32/src/CM_MaterialLedger.h
firmware/esp32/src/CM_MaterialLedger.cpp
firmware/esp32/src/CM_MaterialLedgerWeb.h
firmware/esp32/src/CM_MaterialLedgerWeb.cpp
firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.h
firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.cpp
```

Дополнительные реализации разделены на файлы `CM_Material*`, `CM_RepairPricing*`, `CM_Warehouse*` — перед изменением искать все определения.

`CM_MaterialPersistenceIntegrityAudit` имеет совместимый metrics overload, который считает catalogue/usage/adjustment records в уже выполняемых validation passes. Старый `check(storage)` сохраняется.

## Read-only backup/export и deep integrity

Основная orchestration:

```text
firmware/esp32/src/CM_BackupActivityGuard.h
firmware/esp32/src/CM_BackupActivityGuard.cpp
firmware/esp32/src/CM_BackupExportWeb.h
firmware/esp32/src/CM_BackupExportWeb.cpp
```

Deep audit modules:

```text
firmware/esp32/src/CM_BackupBusinessDataIntegrityAudit.h
firmware/esp32/src/CM_BackupBusinessDataIntegrityAudit.cpp
firmware/esp32/src/CM_WorkshopPersistenceIntegrityAudit.h
firmware/esp32/src/CM_WorkshopPersistenceIntegrityAudit.cpp
firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.h
firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.cpp
firmware/esp32/src/CM_WarehousePersistenceIntegrityAudit.h
firmware/esp32/src/CM_WarehousePersistenceIntegrityAudit.cpp
firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.h
firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.cpp
firmware/esp32/src/CM_PersistentIdIntegrityAudit.h
firmware/esp32/src/CM_PersistentIdIntegrityAudit.cpp
firmware/esp32/src/CM_ConductorSettingsIntegrityAudit.h
firmware/esp32/src/CM_ConductorSettingsIntegrityAudit.cpp
firmware/esp32/src/CM_WindingPersistenceIntegrityAudit.h
firmware/esp32/src/CM_WindingPersistenceIntegrityAudit.cpp
firmware/esp32/src/CM_WindingSessionPersistenceIntegrityAudit.h
firmware/esp32/src/CM_WindingSessionPersistenceIntegrityAudit.cpp
```

`CM_WindingSessionPersistenceIntegrityAudit` — authoritative deep parser/cross-identity audit snapshot/state/spool-selection. Не дублировать его в backup orchestration.

Manifest Stage 0 observability использует already-running passes и не должен добавлять второй full scan только ради counters.

## CI

```text
.github/workflows/esp32-build.yml
.github/workflows/arduino-uno-build.yml
.github/workflows/cmp-protocol-tests.yml
Tests/Protocol/CMakeLists.txt
Tests/Protocol/test_main.cpp
```

GitHub connector для `fetch_commit_workflow_runs` видит только PR-triggered runs, поэтому отсутствие результата не доказывает отсутствие/успех push-run.

## Handoff

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/02_ARCHITECTURE_AND_HARDWARE.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/PROJECT_HANDOFF/04_DATA_STORAGE_API_UI.md
docs/PROJECT_HANDOFF/05_COMPLETED_WORK_LOG.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/07_BACKLOG_AND_DEFERRED.md
docs/PROJECT_HANDOFF/08_WORK_RULES_AND_VERIFICATION.md
docs/PROJECT_HANDOFF/09_KEY_FILES_INDEX.md
docs/PROJECT_HANDOFF/10_SESSION_LOG.md
docs/PROJECT_HANDOFF/11_FULL_BRANCH_AUDIT.md
docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md
```

При переносе в новый чат приоритет чтения: `00` → `12` → `01` → `06` → актуальные исходники; затем остальные handoff-файлы для деталей.
