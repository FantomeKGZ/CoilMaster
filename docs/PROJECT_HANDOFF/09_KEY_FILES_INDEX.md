# Индекс ключевых файлов

Перед редактированием всегда получать актуальное содержимое файла из ветки `cmp-protocol-v1`.

## Главная интеграция ESP32

```text
firmware/esp32/src/main.cpp
```

Содержит `/api/status`, `/api/jobs`, `/api/recovery/acknowledge`, регистрацию web/API, startup persistent-компонентов и lifecycle текущего job.

## Persistent job identity и recovery

```text
firmware/esp32/src/CM_PersistentIdAllocator.h/.cpp
firmware/esp32/src/CM_JobSnapshotStore.h/.cpp
firmware/esp32/src/CM_JobStateStore.h/.cpp
firmware/esp32/src/CM_JobRecovery.h/.cpp
firmware/esp32/src/CM_JobDisplayRecovery.h/.cpp
firmware/esp32/src/CM_JobSpoolSelectionStore.h/.cpp
```

Хранилища:

```text
/data/winding-jobs/id-state.txt
/data/winding-jobs/snapshots/session-<session_id>.json
/data/winding-jobs/spool-selection/session-<session_id>.json
/data/winding-jobs/state/session-<session_id>.json
```

## UART и журнал намотки

```text
firmware/esp32/src/CM_UartEventReceiver.h/.cpp
firmware/esp32/src/CM_WindingJournal.h/.cpp
firmware/esp32/src/CM_WindingJournalSnapshotContext.cpp
```

Журнал:

```text
/data/winding-runs/events.ndjson
```

## История и full validation журнала намотки

```text
firmware/esp32/src/CM_WindingJournalQuery.h
firmware/esp32/src/CM_WindingJournalQuery.cpp
firmware/esp32/src/CM_WindingJournalQueryValidation.cpp
firmware/esp32/src/CM_WindingJournalWeb.h/.cpp
```

`CM_WindingJournalQueryValidation.cpp` содержит authoritative `WindingJournalQuery::validateAll()` / count overload для full-file schema scan до EOF. Не возвращать backup audit к cursor-pagination.

## Программа намотки

```text
firmware/esp32/src/CM_WindingProgramParser.h
```

Единый parser для job creation, registry, similarity и UI-validation.

## Linked job и workshop registry

```text
firmware/esp32/src/CM_JobLinkageRequest.h/.cpp
firmware/esp32/src/CM_JobLinkageResolver.h/.cpp
firmware/esp32/src/CM_RepairRegistry.h/.cpp
firmware/esp32/src/CM_RepairRegistrySimilarity.cpp
firmware/esp32/src/CM_RepairRegistryWeb.h/.cpp
firmware/esp32/src/CM_MotorSimilarityWeb.h/.cpp
```

Данные:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
```

## Production UI

```text
firmware/esp32/web/mobile/
firmware/esp32/web/desktop/
```

Основной production UI flow уже собран. Следующий обязательный внешний этап — hardware E2E ESP32 + Arduino, а не повторная реализация repair/winding UI.

## Склад, материалы и costing

Ключевые группы:

```text
firmware/esp32/src/CM_Warehouse*.h/.cpp
firmware/esp32/src/CM_Material*.h/.cpp
firmware/esp32/src/CM_RepairCosting*.h/.cpp
firmware/esp32/src/CM_RepairPricing*.h/.cpp
```

`CM_MaterialPersistenceIntegrityAudit` имеет совместимый metrics overload для catalogue/usage/adjustment counts; старый `check(storage)` сохранён.

## Read-only backup/export и deep integrity

Orchestration:

```text
firmware/esp32/src/CM_BackupActivityGuard.h/.cpp
firmware/esp32/src/CM_BackupExportWeb.h/.cpp
```

Deep audit modules:

```text
firmware/esp32/src/CM_BackupBusinessDataIntegrityAudit.h/.cpp
firmware/esp32/src/CM_WorkshopPersistenceIntegrityAudit.h/.cpp
firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.h/.cpp
firmware/esp32/src/CM_WarehousePersistenceIntegrityAudit.h/.cpp
firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.h/.cpp
firmware/esp32/src/CM_PersistentIdIntegrityAudit.h/.cpp
firmware/esp32/src/CM_ConductorSettingsIntegrityAudit.h/.cpp
firmware/esp32/src/CM_WindingPersistenceIntegrityAudit.h/.cpp
firmware/esp32/src/CM_WindingSessionPersistenceIntegrityAudit.h/.cpp
```

Ключевые правила:

- `CM_WindingPersistenceIntegrityAudit` использует `WindingJournalQuery::validateAll()` + отдельный transition audit; cursor-pagination full scan там отсутствует.
- `CM_WindingSessionPersistenceIntegrityAudit` — authoritative deep parser/cross-identity audit snapshot/state/spool-selection. Не дублировать его.
- Session audit теперь имеет совместимый metrics overload `WindingSessionPersistenceAuditMetrics`; старый `check(storage)` сохранён.
- Metrics overload возвращает `snapshotFileCount`, `stateFileCount`, `spoolSelectionFileCount` только после полного успешного deep session audit; partial counts при failure не публикуются.
- `CM_BackupExportWeb.cpp` публикует Stage 0 material/business/winding/warehouse record counts, session file counts и `snapshot_stability_duration_ms` только через уже выполняемые authoritative passes.
- `BackupActivityGuard::Safe` gating не ослаблять: heavy deep scan не выполняется во время active winding.

## Performance/rotation

```text
docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
```

До hardware benchmark не делать произвольный rotation threshold, persistent optimistic cache, database migration или Stage 1 duplicate-scan refactor без отдельной correctness-причины.

## HTTP semantics

```text
docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md
```

## CI

```text
.github/workflows/esp32-build.yml
.github/workflows/arduino-uno-build.yml
.github/workflows/cmp-protocol-tests.yml
Tests/Protocol/CMakeLists.txt
Tests/Protocol/test_main.cpp
```

Отсутствие workflow/status результата в GitHub connector не доказывает GREEN push-run.

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

При переносе в новый чат: `00` → `12` → `01` → `06` → актуальные исходники; затем остальные handoff-файлы.
