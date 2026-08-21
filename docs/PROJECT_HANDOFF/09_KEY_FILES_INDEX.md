# Индекс ключевых файлов

Дата актуализации: **2026-08-21**  
Ветка: `cmp-protocol-v1`

Перед редактированием любого existing file получать его актуальное содержимое и current blob SHA.

## Current entrypoints

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Do not use old `00 -> 28 -> ...` or checkpoint-24/38 continuation orders anymore. Older checkpoints are history/evidence only.

## Arduino production

```text
firmware/arduino/src/main.cpp
Core/
Arduino/
Arduino/Config/CM_Pins.h
Arduino/CM_UartEventTransport.h/.cpp
Arduino/CM_SsrController.*
Arduino/CM_HallTurnSource.*
Arduino/CM_HallCalibrationService.*
```

Arduino owns physical START, SSR, Hall counting, local state machine and generation of RUN events.

## ESP32 main integration

```text
firmware/esp32/src/main.cpp
firmware/esp32/src/CM_StaticSiteServer.h/.cpp
```

ESP32 owns service/data/UI orchestration, job persistence/delivery, registry, warehouse/materials/costing, backup/restore, network and diagnostics.

## Production CMP1 / job cancel

```text
Arduino/CM_UartEventTransport.h/.cpp
firmware/esp32/src/CM_UartEventReceiver.h/.cpp
Shared/CMP1Text/CM_Cmp1Crc.h
firmware/arduino/src/main.cpp
```

Current cancel/recovery implementation includes no-run cancel, idempotent `ALREADY_CLEAR`, safe `ALL_CLEAR` fallback and no automatic run-completion semantics.

`Shared/Protocol/` is older binary host-test protocol and not production CMP1.

## Persistent job identity/state/recovery

```text
CM_PersistentIdAllocator.*
CM_JobSnapshotStore.*
CM_JobStateStore.*
CM_JobStateRemoteCancel.cpp
CM_JobStateDismiss.cpp
CM_JobRecovery.*
CM_JobDisplayRecovery.*
CM_JobSpoolSelectionStore.*
```

The obsolete duplicate `CM_JobStateStoreLifecycle.cpp` was removed in `e35c4bfe`; do not recreate its duplicate method definitions.

Relevant data:

```text
/data/winding-jobs/id-state.txt
/data/winding-jobs/snapshots/session-<id>.json
/data/winding-jobs/spool-selection/session-<id>.json
/data/winding-jobs/state/session-<id>.json
```

## Winding journal

```text
CM_WindingJournal.*
CM_WindingJournalQuery.h/.cpp
CM_WindingJournalQueryValidation.cpp
CM_WindingJournalTransitionAudit.*
CM_WindingJournalWeb.*
```

Authoritative full validation:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

Do not reintroduce cursor-pagination as full authoritative validation.

## Workshop / motors / repairs

```text
CM_RepairRegistry.*
CM_RepairRegistryWeb.*
CM_RepairRegistryLookupWeb.*
CM_MotorSimilarityWeb.*
CM_WindingProgramParser.h
```

Data root:

```text
/data/workshop/
```

Motor import format: `docs/MOTOR_IMPORT_FORMAT.md`.

## Warehouse / KG_FIRST / material writeoff

```text
CM_Warehouse*.h/.cpp
CM_Material*.h/.cpp
CM_RepairCosting*.h/.cpp
CM_RepairPricing*.h/.cpp
CM_JobSpoolSelectionStore.*
CM_WindingJournalQuery*
```

Current new-consumption provenance:

```text
source_session_id + source_run_id mandatory
spool_id optional only for approved KG_FIRST unallocated/manual path
```

If a spool is used, exact spool provenance and decrement remain exact. `RUN_COMPLETED` never auto-writes off material.

## Backup / deep integrity

Orchestration:

```text
CM_BackupActivityGuard.*
CM_BackupExportWeb.*
CM_RemoteBackupSettings.*
CM_RemoteBackupTransfer.*
CM_RemoteBackupWeb.*
```

Key integrity owners:

```text
CM_BackupBusinessDataIntegrityAudit.*
CM_WorkshopPersistenceIntegrityAudit.*
CM_MaterialPersistenceIntegrityAudit.*
CM_WarehousePersistenceIntegrityAudit.*
CM_WarehouseMovementIntegrityAudit.*
CM_PersistentIdIntegrityAudit.*
CM_ConductorSettingsIntegrityAudit.*
CM_WindingPersistenceIntegrityAudit.*
CM_WindingSessionPersistenceIntegrityAudit.*
```

`WindingSessionPersistenceIntegrityAudit` owns authoritative read-only session preflight. Backup export must not reintroduce the removed duplicate full directory scan.

Historical Stage-0 metric counts in older checkpoints are historical snapshots; use current code/manifest contract rather than copying old numeric counts.

## Network / FTP / diagnostics

```text
CM_NetworkProfileStore.*
CM_NetworkManager.*
CM_NetworkWeb.*
CM_RemoteBackupSettings.*
CM_RemoteBackupTransfer.*
CM_RemoteBackupWeb.*
CM_WebRecoveryFtpServer.*
CM_StorageDiagnosticsWeb.*
```

Incoming recovery FTP is restricted to `/web`; outbound remote backup is a separate client path.

## Web UI

```text
firmware/esp32/web/desktop/
firmware/esp32/web/mobile/
firmware/esp32/web/shared/
Tests/Web/
```

Dynamic markup may live in shared JS. Regression tests should inspect the actual source of generated markup rather than require it in static HTML.

## Build / CI

```text
platformio.ini
.github/workflows/arduino-uno-build.yml
.github/workflows/esp32-build.yml
.github/workflows/cmp-protocol-tests.yml
Tests/Protocol/
Tests/CMP1Text/
Tests/Web/
```

Verified production baseline `e35c4bfe`:

```text
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

Arduino Uno Build and current-head hardware acceptance remain separate evidence.

## Hardware references

```text
docs/HARDWARE_REFERENCE/
docs/PROJECT_HANDOFF/02_ARCHITECTURE_AND_HARDWARE.md
Engineering/Hardware/
```

Known UART pairing remains Arduino A1/A2 through level shifting to ESP32 GPIO16/17, with common signal ground.

## Historical docs

```text
docs/PROJECT_HANDOFF/05_COMPLETED_WORK_LOG.md
docs/PROJECT_HANDOFF/10_SESSION_LOG.md
older numbered checkpoints
Docs/
```

These are useful for archaeology and old verification evidence only. They do not select current active work.
