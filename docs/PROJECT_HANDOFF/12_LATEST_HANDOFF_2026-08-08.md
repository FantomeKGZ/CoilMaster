# CoilMaster — полный handoff на 2026-08-08

Ветка: `cmp-protocol-v1`  
Репозиторий: `FantomeKGZ/CoilMaster`  
Этот файл — свежая точка входа после backup-integrity, HTTP semantics и NDJSON performance review.

## 1. Главные правила продолжения

- Источник истины — только текущий код ветки `cmp-protocol-v1`.
- Не использовать `main` как источник кода или архитектурных решений.
- Перед каждым edit/delete заново fetch текущего файла из `cmp-protocol-v1` и использовать его blob SHA.
- Новый файл сначала проверять на отсутствие точного пути.
- Не считать документацию или отсутствие workflow-run доказательством зелёного CI.
- Не заявлять hardware E2E без реального стенда ESP32 + Arduino и подтверждения пользователя.
- Не делать auto physical START, auto resume после reboot, direct SSR с ESP32/WEB или automatic wire writeoff только по `RUN_COMPLETED`.

## 2. Production workflow, который уже собран

```text
client
→ motor с authoritative coil_program
→ repair OPEN
→ costing
→ linked winding job
→ обязательный exact spool_id
→ immutable job snapshot
→ immutable spool-selection
→ UART delivery
→ JOB_ACK ACCEPTED
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ winding history
→ ручной write-off провода с фактическим весом
→ source_session_id + source_run_id provenance
→ дополнительные материалы / pricing
→ read-only finalization preflight
→ CLOSED
→ read-only archive
→ monthly report
→ read-only backup/export
```

Физический START остаётся аппаратным действием. `RUN_COMPLETED` сам склад не меняет.

## 3. Persistent winding и recovery — уже реализовано

Реализованы и не должны начинаться заново:

- persistent `job_id/session_id` allocator;
- immutable job snapshot;
- immutable exact spool selection;
- persisted runtime state;
- fail-safe reboot recovery;
- manual-review acknowledgement;
- strict repair/motor linkage;
- authoritative `coil_program`;
- единый `CM_WindingProgramParser`;
- winding journal schema 2;
- strict writer scans;
- read-only winding history с cursor pagination для пользовательского API;
- full-file authoritative `WindingJournalQuery::validateAll()` для integrity;
- semantic transition audit `RUN_STARTED → RUN_COMPLETED`;
- runtime dynamic microSD readiness.

Ключевые файлы:

```text
firmware/esp32/src/CM_PersistentIdAllocator.*
firmware/esp32/src/CM_JobSnapshotStore.*
firmware/esp32/src/CM_JobSpoolSelectionStore.*
firmware/esp32/src/CM_JobStateStore.*
firmware/esp32/src/CM_JobRecovery.*
firmware/esp32/src/CM_JobLinkageResolver.*
firmware/esp32/src/CM_WindingProgramParser.h
firmware/esp32/src/CM_WindingJournal.*
firmware/esp32/src/CM_WindingJournalQuery.*
firmware/esp32/src/CM_WindingJournalTransitionAudit.*
```

Важно: `CM_WindingPersistenceIntegrityAudit` уже использует `validateAll()` + `WindingJournalTransitionAudit::validate()`. Старого cursor-pagination полного scan там нет, поэтому дополнительная правка этого файла не нужна.

## 4. Repair lifecycle и CLOSED invariant

`OPEN → CLOSED` append-only lifecycle реализован server-side.

CLOSED repair запрещает:

- новый linked winding;
- новое wire writeoff;
- material usage;
- pricing revision.

Истории остаются read-only.

Close блокируется, если есть unfinished/recovery winding state или не выполнены finalization checks.

Для новых completed linked runs с immutable spool-selection требуется ручной CONFIRMED wire writeoff именно для `(source_session_id, source_run_id)`.

## 5. Warehouse / material / costing integrity

Warehouse:

- recoverable spool-file swap;
- writeoff transaction `PENDING → CONFIRMED | ABORTED`;
- startup recovery;
- strict movement parser;
- exact spool identity;
- provenance `source_session_id + source_run_id`;
- duplicate source-run CONFIRMED writeoff запрещён;
- dynamic storage readiness.

Material ledger:

- recoverable material file swap;
- usage/adjustment pending recovery;
- strict catalogue/history parsing;
- formula validation;
- KGS policy в production path;
- dynamic storage readiness.

RepairCosting:

- strict warehouse transaction integrity;
- material usage formula validation;
- pricing history validation;
- checked overflow;
- consistent currency;
- fail-closed corrupted dependencies.

## 6. UI и operator workflow

Mobile/desktop уже имеют:

- clients, motors, repairs;
- linked winding;
- exact spool selection;
- lifecycle status;
- winding history;
- writeoff history с session/run provenance;
- costing;
- CLOSED read-only mode;
- repair archive filters;
- finalization preflight;
- monthly reports;
- backup/export pages.

Новые linked jobs требуют exact active CU/AL spool с положительным остатком. Backend повторно проверяет spool identity перед job creation.

## 7. Read-only backup/export — deep audit охватывает весь whitelist

API:

```text
GET /api/backup/manifest
GET /api/backup/file?name=...
GET /api/backup/sessions
GET /api/backup/session-file?kind=...&session_id=...
```

Произвольные filesystem paths запрещены.

`export_allowed` отвечает за то, безопасно ли выполнять тяжёлый export сейчас.

Глубокий integrity audit выполняется только когда `BackupActivityGuard` доказывает `Safe`. Во время active winding manifest не запускает тяжёлые scans и возвращает:

```text
snapshot_stability_checked=false
snapshot_stable=null
```

При safe state `snapshot_stable=true` требует успешной проверки **всего static export whitelist**:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
/data/winding-runs/events.ndjson
/data/warehouse/spools.ndjson
/data/warehouse/movements.ndjson
/data/warehouse/price.ndjson
/data/materials/materials.ndjson
/data/materials/usage.ndjson
/data/materials/adjustments.ndjson
/data/repairs/pricing.ndjson
/data/winding-jobs/id-state.txt
/data/winding-jobs/id-state.bak          # optional
/data/settings/conductor-calculator.ndjson
```

Дополнительно проверяются необходимые recovery/session invariants:

- recovery markers materials;
- warehouse spool swap markers;
- allocator `id-state.txt`, optional `.bak`, отсутствие `id-state.tmp`;
- conductor settings `conductor-calculator.ndjson`, отсутствие `.tmp/.bak` recovery residue;
- workshop clients/motors/repairs + repair-status references;
- repair pricing + repair references;
- materials catalogue/usage/adjustments + arithmetic + references;
- winding journal full schema validation до EOF;
- winding transition semantics;
- warehouse spools/price/movements transaction/reference integrity;
- canonical session directories;
- содержимое каждого snapshot/state/spool-selection;
- cross-file `job/session/repair/motor/spool` identity.

Задействованные audit-модули:

```text
CM_BackupBusinessDataIntegrityAudit.*
CM_MaterialPersistenceIntegrityAudit.*
CM_WorkshopPersistenceIntegrityAudit.*
CM_RepairPricingIntegrityAudit.*
CM_WindingPersistenceIntegrityAudit.*
CM_WarehousePersistenceIntegrityAudit.*
CM_WarehouseMovementIntegrityAudit.*
CM_PersistentIdIntegrityAudit.*
CM_ConductorSettingsIntegrityAudit.*
CM_WindingSessionPersistenceIntegrityAudit.*
```

Статический compile-safety audit include-зависимостей выполнен: audit headers содержат собственный `FS.h`; реализации, использующие Arduino `String/File/isDigit`, имеют требуемую dependency chain; ESP32 PlatformIO filter компилирует все `firmware/esp32/src/*.cpp`. Missing include в проверенном наборе не найден, но это не заменяет фактический Actions build.

## 8. HTTP/error semantics audit

Подробный документ:

```text
docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md
```

Каноническая карта:

```text
400 malformed / missing request field
404 requested allowed resource absent
409 syntactically valid request blocked by machine/domain state
500 persisted read/integrity failure
503 storage/service dependency unavailable
```

Manifest намеренно является status endpoint: active winding выражается `200` + `export_allowed=false`, а direct heavy export блокируется `409`.

Найден один реальный run-level gap: пустые, но присутствующие `source_session_id` и `source_run_id` могли восприниматься как отсутствие provenance и перейти в legacy path.

Исправлено:

```text
362fcb7daa8f883f57de4867c06c42f06e45b613  Reject empty run provenance fields
```

Теперь если run-level параметры присутствуют, они обязаны присутствовать вместе, быть непустыми и валидными non-zero IDs; иначе `400` без записи.

## 9. NDJSON performance/rotation strategy

Подробный документ:

```text
docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
```

Решение: **не мигрировать преждевременно в БД**.

Найденные hotspots:

- deep backup full scans;
- повторные cross-file reference scans в business/material/warehouse audits;
- costing/finalization/history по растущим append-only NDJSON;
- winding journal имеет отдельные schema + transition full scans, но schema validation уже не строит cursor JSON pages.

Порядок дальнейшей оптимизации:

```text
1. измерить file size / record count / scan latency
2. убрать доказанно дорогие повторные scans bounded per-request indexes
3. при необходимости — recoverable rotation immutable append histories
4. read-only summary snapshots для reports/dashboard
5. БД только если измерения покажут недостаточность предыдущих этапов
```

Не делать unbounded RAM mirror всех NDJSON на ESP32. Rotation должна сохранять provenance/global IDs, fail-closed cross-segment validation и полноту backup/export.

## 10. Текущая серия коммитов после deep backup блока

```text
362fcb7d  Reject empty run provenance fields
5544f07f  Document backup and run HTTP semantics audit
23737ef5  Plan NDJSON growth and rotation strategy
6b9762f9  Refresh backup integrity current state
03dbc1fe  Advance active work after integrity review
```

Этот handoff-коммит идёт после них и синхронизирует точку продолжения.

## 11. Последняя найденная CI-ошибка и исправление

Ранее пользователь дал Actions run:

```text
https://github.com/FantomeKGZ/CoilMaster/actions/runs/31243187630
```

Run checkout был на commit:

```text
78ac24533f1157080bd2163990dbdb0b2577807c
```

`build-esp32` job id:

```text
93067378338
```

Реальная ошибка из полного лога:

```text
firmware/esp32/src/CM_MaterialLedger.cpp:705:1: error: expected '}' at end of input
note: to match namespace CM opening brace
```

Исправлено:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8  Fix MaterialLedger namespace closure
```

Для нового кодового commit `362fcb7d...` текущий connector не вернул combined statuses или PR-triggered workflow run. Это **не доказательство GREEN и не доказательство failure**. При доступном push-run проверять реальный job/result.

## 12. Hardware E2E — обязательный внешний этап

Реальный стенд должен пройти минимум:

```text
client + motor + repair
→ warehouse price + active CU/AL spool
→ linked winding + exact spool_id
→ JOB_ACK ACCEPTED
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ winding history + immutable spool identity
→ manual wire writeoff
→ source_session_id + source_run_id
→ costing
→ finalization preflight
→ CLOSED
→ archive/report
→ stable backup manifest/export
```

Fault scenarios:

- microSD unavailable before job;
- runtime microSD loss;
- reboot after ACCEPTED before START;
- reboot after RUN_STARTED;
- corrupt snapshot/state/spool-selection/journal;
- corrupt warehouse/material/pricing/workshop files;
- dangling warehouse PENDING;
- material pending/swap markers;
- duplicate writeoff for same `(session_id, run_id)`;
- close without manual wire coverage;
- backup while winding active;
- backup with temp/pending/corrupt persisted state.

Repository review и CI не заменяют этот этап. Не считать физическое поведение ESP32 + Arduino проверенным, пока пользователь сам это не подтвердит.

## 13. Deferred product work

Не начинать без явной потребности:

- analogue/unassigned winding production model;
- automatic material writeoff;
- automatic safe resume;
- database migration;
- direct SSR control с ESP32/WEB.

## 14. Рекомендуемый порядок чтения в новом чате

1. `docs/PROJECT_HANDOFF/00_READ_FIRST.md`
2. этот файл `12_LATEST_HANDOFF_2026-08-08.md`
3. `01_CURRENT_STATE.md`
4. `06_ACTIVE_WORK_AND_NEXT_STEPS.md`
5. актуальные исходники конкретного следующего изменения
6. `docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md`
7. `docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md`
8. `09_KEY_FILES_INDEX.md`
9. `08_WORK_RULES_AND_VERIFICATION.md`

`11_FULL_BRANCH_AUDIT.md` считать исторической картой, не текущим source of truth.

## 15. Точная точка продолжения

После этого review:

1. Не повторять deep backup-integrity block.
2. Не переписывать `CM_WindingPersistenceIntegrityAudit`: `validateAll()` уже authoritative и cursor full scan отсутствует.
3. При наличии нового ESP32 Actions run проверить фактический build для кодового head и чинить первую реальную compile/link error, если она есть.
4. Следующий repo-reviewable performance шаг — low-cost observability file-size/record-count/scan-latency, прежде чем внедрять rotation.
5. Hardware E2E остаётся обязательным внешним этапом и не отмечается выполненным без подтверждения пользователя.
