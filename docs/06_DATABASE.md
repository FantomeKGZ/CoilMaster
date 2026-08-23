# CoilMaster — current persisted data model

## 1. Что это за модель

CoilMaster сейчас не использует SQL/relational database как authoritative runtime store. Production state хранится на microSD под `/data` через domain-owned JSON/NDJSON/files.

Поэтому этот документ описывает **logical identities, ownership и persistence contracts**, а не fictitious SQL schema. При расхождении authoritative являются current `cmp-protocol-v1` source + integrity/backup tests.

Основное правило:

```text
one domain owner -> explicit persisted files -> validator/audit -> backup/restore coverage
```

Нельзя добавлять production file/field, не проверив reader, validation, compatibility, integrity, backup и restore.

## 2. High-level business graph

```text
Client
  -> Repair
      -> Motor
      -> linked Winding Job/Session
          -> immutable Snapshot
          -> exact immutable Spool Selection
          -> Winding Run evidence
              -> explicit Manual Material Writeoff
      -> auxiliary Material Usage
      -> Costing / Pricing evidence
      -> CLOSED lifecycle evidence
```

Motor является технологической сущностью; связь с конкретным клиентом/работой проходит через repair/business records, а не через прямое physical-movement ownership.

## 3. Workshop registry

Owner family:

```text
CM_RepairRegistry.*
CM_RepairRegistryWeb.*
CM_RepairLifecycle.h
CM_WorkshopPersistenceIntegrityAudit.*
```

Known runtime root:

```text
/data/workshop/
```

Workshop domain хранит client/motor/repair records и lifecycle evidence.

Текущий repair lifecycle для production completion не использует старую enum-модель `ACCEPTED/IN_PROGRESS/READY/ISSUED/ARCHIVED`. `CM_RepairLifecycle` считает repair OPEN, пока для exact `repair_id` нет валидной unique `CLOSED` записи в:

```text
/data/workshop/repair-status.ndjson
```

Durable CLOSED evidence содержит exact `repair_id`, status `CLOSED` и `closed_at`.

## 4. Winding identities

Ключевые IDs:

```text
job_id
session_id
run_id
repair_id
motor_id
```

Responsibilities:

- `job_id` — persisted remote job identity;
- `session_id` — identity linked winding session/job persistence;
- `run_id` — отдельный физический execution attempt;
- `repair_id` / `motor_id` — business linkage.

Повтор полного winding program создает новый `run_id`; authoritative RUN records не сливаются физически ради UI aggregation.

## 5. Winding journal

Owner family:

```text
CM_WindingJournal.*
CM_WindingJournalQuery*
CM_WindingJournalWeb.*
CM_WindingPersistenceIntegrityAudit.*
```

Known event log:

```text
/data/winding-runs/events.ndjson
```

Фактический run evidence приходит от Arduino:

```text
RUN_STARTED(session_id, run_id, ...)
RUN_COMPLETED(session_id, run_id, ...)
```

Duplicate/retry одного exact event/run не должен создавать второй фактический run.

`RUN_COMPLETED` является execution evidence, но **не warehouse mutation**.

## 6. Linked job persistence

Runtime root:

```text
/data/winding-jobs/
```

Main owners:

```text
CM_PersistentIdAllocator.*
CM_JobSnapshotStore.*
CM_JobStateStore.*
CM_JobSpoolSelectionStore.*
CM_JobRecovery.*
CM_JobDisplayRecovery.*
CM_WindingSessionPersistenceIntegrityAudit.*
```

### Snapshot

Immutable job/business parameters, сохраненные до physical execution boundary.

### State

Operational job lifecycle/delivery/recovery evidence. State mutation не имеет права удалять immutable run/snapshot/material provenance.

### Exact spool selection

Current linked production требует immutable selection до UART delivery. `JobSpoolSelection` valid only when all critical identities/material fields nonzero/valid, включая:

```text
job_id
session_id
repair_id
motor_id
spool_id
diameterHundredthsMm
weightAtSelectionGrams
wireType = CU|AL
```

`spool_id == 0` не является valid current linked selection.

## 7. Crash-residue boundaries

Не все `.tmp/.bak` одинаковы:

```text
JobStateStore .tmp/.bak
  fail-closed replacement/recovery evidence

JobSpoolSelectionStore .json.tmp
  bounded pre-UART recovery policy

JobSnapshotStore .json.tmp
  fail-closed / REVIEW resilience boundary
```

Cleanup не должен автоматически удалять/promote residue только ради единообразия.

## 8. Warehouse wire model

Authoritative wire inventory — spool-based warehouse, а не старый generic `WarehouseItem/item_id` draft.

Owner family:

```text
CM_WarehouseStore.*
CM_WarehouseSpoolWeb.cpp
CM_WarehouseWriteOff*.cpp
CM_WarehousePersistenceIntegrityAudit.*
```

Physical wire spool имеет exact immutable `spool_id`; authoritative identity включает material (`CU`/`AL`), diameter и current mass.

`CM_WarehouseLegacySpoolMaterial.cpp` — live migration owner для старых ACTIVE spools без `wire_type`, поэтому остается `KEEP`.

## 9. Manual wire writeoff

Current linked production writeoff требует exact provenance:

```text
repair_id
source_session_id
source_run_id
exact immutable spool_id
```

До confirmed mutation WarehouseStore fail-closed проверяет:

- store ready;
- repair exists and remains OPEN;
- immutable selection exists for exact session;
- selection repair/spool match request;
- exact source run is COMPLETED;
- no previous confirmed writeoff for that source run;
- spool identity/stock/price are valid.

Transaction shape:

```text
PENDING movement
-> spool mutation
-> CONFIRMED movement
```

Rollback-success path записывает `ABORTED`; ambiguous mutation переводит store в not-ready/fail-closed until reconciliation.

Historical `UNALLOCATED` KG_FIRST records остаются read/audit/recovery compatibility evidence only. Current linked KG_FIRST requires exact selected `spool_id`.

## 10. Materials domain

Auxiliary materials are a separate persisted domain, not aliases of wire spools.

Owners:

```text
CM_Material*.h/.cpp
CM_MaterialPersistenceIntegrityAudit.*
```

Known root:

```text
/data/materials/
```

Usage/history/adjustments должны сохранять historical cost/evidence и не переписывать прошлое при изменении current catalogue price.

## 11. Costing / pricing

Owners:

```text
CM_RepairCosting*.h/.cpp
CM_RepairPricing*.h/.cpp
```

Costing/finalization строятся из persisted authoritative repair, exact run/writeoff/material and pricing evidence. UI-derived totals не являются единственным source of truth.

Repair не должен закрываться, если finalization preflight не подтверждает required exact persisted evidence.

## 12. Autonomous/local winding archive

Owners:

```text
CM_AutonomousWindingArchive.h/.cpp
CM_AutonomousWindingArchiveAssign.cpp
CM_AutonomousWindingArchivePage.cpp
CM_AutonomousWindingArchiveIntegrity.cpp
CM_AutonomousWindingWeb.*
```

Known root:

```text
/data/autonomous-windings/
```

Local program events могут существовать до business assignment. Assignment создает linkage; исходный execution evidence остается append-only и не переписывается.

Split `.cpp` implementations реализуют методы одного `AutonomousWindingArchive` owner и не являются orphan duplicates.

## 13. Network/settings persisted state

Examples:

```text
CM_NetworkProfileStore.*
CM_RemoteBackupSettings.*
CM_ConductorSettingsStore.cpp
```

Mutable single-file settings используют bounded atomic-replacement/recovery semantics. Corrupt/torn persisted settings должны fail-closed или recover only по owner-defined validated policy.

## 14. Backup / restore coverage

Production persisted domains должны входить в whitelist/integrity/restore policy через owners:

```text
CM_BackupBusinessDataIntegrityAudit.*
CM_WorkshopPersistenceIntegrityAudit.*
CM_WarehousePersistenceIntegrityAudit.*
CM_MaterialPersistenceIntegrityAudit.*
CM_WindingPersistenceIntegrityAudit.*
CM_WindingSessionPersistenceIntegrityAudit.*
CM_ConductorSettingsIntegrityAudit.*
```

Restore rules:

- explicit operator action only;
- strict preflight;
- transactional apply/rollback;
- persisted stale evidence blocks unsafe continuation;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion/truncation under storage pressure.

## 15. Reference motor catalogue is not production database

Repository source data:

```text
data/motor_catalog/**/*.source.json
```

is transformed by:

```text
tools/build_motor_reference.py
```

into read-only Web reference dataset:

```text
firmware/esp32/web/reference/motor-reference.json
```

Reference records are explicitly reference-only and must not silently become executable `coil_program`/working motor records.

## 16. Web/API representation

Web pages are consumers of authoritative ESP32 stores. They do not own persisted truth.

When changing a persisted contract, review together:

```text
STORE/WRITER
-> authoritative READER
-> integrity AUDIT
-> backup/restore whitelist
-> *Web.cpp API
-> desktop/mobile/shared UI
-> Tests/Web regression
```

## 17. Data-change checklist

Before adding/changing persisted data:

1. exact owner identified;
2. stable identity/provenance defined;
3. writer validates all mandatory fields;
4. authoritative reader rejects malformed/ambiguous state;
5. old valid records remain intentionally readable or have explicit migration;
6. integrity audit covers syntax + cross-reference semantics;
7. backup/export includes the path;
8. restore validates and rolls back transactionally;
9. no automatic deletion/truncation shortcut;
10. API/UI preserve the same semantics;
11. targeted regression test exists;
12. hardware verification added only if behavior truly crosses physical boundary.

This checklist is the current “database schema discipline” for CoilMaster.
