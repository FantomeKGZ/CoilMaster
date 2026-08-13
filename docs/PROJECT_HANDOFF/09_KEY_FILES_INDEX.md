# Индекс ключевых файлов

Перед редактированием всегда получать актуальное содержимое файла из ветки `cmp-protocol-v1`.

## Главная интеграция ESP32

```text
firmware/esp32/src/main.cpp
```

Содержит `/api/status`, `/api/jobs`, `/api/recovery/acknowledge`, регистрацию web/API, startup persistent-компонентов и lifecycle текущего job.

## Сеть, исходящий backup FTP и входящий recovery FTP

```text
firmware/esp32/src/CM_NetworkProfileStore.h/.cpp
firmware/esp32/src/CM_NetworkManager.h/.cpp
firmware/esp32/src/CM_NetworkWeb.h/.cpp
firmware/esp32/src/CM_RemoteBackupSettings.h/.cpp
firmware/esp32/src/CM_RemoteBackupTransfer.h/.cpp
firmware/esp32/src/CM_RemoteBackupWeb.h/.cpp
firmware/esp32/src/CM_WebRecoveryFtpServer.h/.cpp
firmware/esp32/src/CM_StaticSiteServer.h/.cpp
firmware/esp32/web/shared/settings-wifi.js
firmware/esp32/web/shared/settings-remote-backup.js
```

`CM_WebRecoveryFtpServer` — отдельный одноклиентный входящий FTP только для
`/web`. Не путать с `CM_RemoteBackupTransfer`, который является исходящим FTP-
клиентом для резервных копий на USB-хранилище роутера. Оба пути используют
общий fail-closed runtime activity probe, но имеют разные корни и назначение.

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

`CM_PersistentIdIntegrityAudit` имеет совместимый `PersistentIdIntegrityAuditMetrics` overload. Он возвращает validated `lastAllocatedId` из уже выполняемой проверки `last_job_id == last_session_id`; старый `check(storage)` сохранён.

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

## Flat persisted JSON validator

```text
firmware/esp32/src/CM_FlatJsonObjectValidator.h
```

Header-only syntax validator для уже прочитанной flat JSON object строки. Используется workshop/pricing/material/warehouse/settings authoritative readers. Не вызывать его повторно внутри O(n²)/O(n*m) identity/reference scans, если соответствующий authoritative outer pass уже проверяет каждую строку.

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

`CM_RepairRegistry` теперь fail-closed проверяет flat JSON syntax в authoritative/runtime reads, поэтому corrupted persisted line не должна молча попасть в JSON API.

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

`CM_MaterialPersistenceIntegrityAudit` имеет совместимый metrics overload для catalogue/usage/adjustment counts; старый `check(storage)` сохранён. Authoritative material/usage/adjustment passes также требуют valid flat JSON syntax.

`CM_WarehousePersistenceIntegrityAudit` имеет совместимый `WarehousePersistenceAuditMetrics` overload для spool/price counts; старый `check(storage)` сохранён. Counts публикуются только после полного warehouse persistence audit, включая movement-reference validation. Spool/price/movement authoritative passes требуют valid flat JSON syntax.

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
- Session audit имеет совместимый metrics overload `WindingSessionPersistenceAuditMetrics`; старый `check(storage)` сохранён.
- Session metrics возвращают `snapshotFileCount`, `stateFileCount`, `spoolSelectionFileCount` только после полного successful deep audit; partial counts при failure не публикуются.
- Те же session passes возвращают `snapshotTotalBytes`, `stateTotalBytes`, `spoolSelectionTotalBytes`; 32-bit telemetry overflow не меняет integrity result.
- `CM_PersistentIdIntegrityAudit` имеет совместимый `PersistentIdIntegrityAuditMetrics` overload; `lastAllocatedId` публикуется только после successful main/optional-backup audit.
- `CM_BackupBusinessDataIntegrityAudit` имеет совместимый `BackupBusinessDataAuditMetrics` overload; business counts берутся из существующих validation passes без telemetry-only full scan.
- Business/material/warehouse/settings flat JSON authoritative rows теперь проходят `CM_FlatJsonObjectValidator`; malformed flat JSON должен fail closed.
- Strict flat JSON validation выполняется на outer pass; repeated identity/reference scans остаются identity-focused, чтобы не умножать parser CPU внутри уже известных O(n²)/O(n*m) paths.
- `CM_WarehousePersistenceIntegrityAudit` имеет совместимый `WarehousePersistenceAuditMetrics` overload; spool/price counts считаются в authoritative passes, partial metrics при failure не публикуются.
- `CM_BackupExportWeb.cpp` публикует **29 Stage 0 metrics**: total duration, 9 per-domain timings, allocator high-water, material/business/winding/warehouse record counts, session file counts и session byte totals.
- Per-domain timing использует `millis()` вокруг уже существующих audit calls; дополнительного SD I/O нет.
- `winding_session_directory_scan_duration_ms` измеряет preliminary directory scan отдельно от `winding_session_persistence_audit_duration_ms`, чтобы benchmark показал цену обоих passes до Stage 1 refactor.
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
