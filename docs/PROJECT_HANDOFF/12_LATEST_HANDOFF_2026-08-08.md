# CoilMaster — полный handoff на 2026-08-08

Ветка: `cmp-protocol-v1`  
Репозиторий: `FantomeKGZ/CoilMaster`  
Этот файл создан как максимально свежая точка входа для нового чата после большой серии изменений backend/UI/integrity/backup.

## 1. Главные правила продолжения

- Источник истины — только текущий код ветки `cmp-protocol-v1`.
- Не использовать `main` как источник кода или архитектурных решений.
- Перед каждым edit/delete заново fetch текущего файла из `cmp-protocol-v1` и использовать его blob SHA.
- Новый файл сначала проверять на отсутствие точного пути.
- Не считать документацию или отсутствие workflow-run доказательством зелёного CI.
- Не заявлять hardware E2E без реального стенда ESP32 + Arduino.
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
- full read-only winding history с cursor pagination;
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

## 7. Read-only backup/export — текущий расширенный контракт

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

Когда состояние safe, `snapshot_stable=true` требует успешной проверки практически всего whitelist persistent-набора:

- recovery markers materials;
- warehouse spool swap markers;
- persistent allocator `id-state.txt`, optional `.bak`, отсутствие `.tmp`;
- `conductor.json`, отсутствие `conductor.tmp`;
- workshop clients/motors/repairs;
- repair-status closure history;
- repair-pricing и pricing→repair reference;
- materials catalogue;
- material usage/adjustments + arithmetic + references;
- winding journal full schema validation;
- winding transition semantics;
- warehouse spools;
- warehouse price;
- warehouse movements transaction chain;
- session directories;
- содержимое snapshot/state/spool-selection;
- cross-file job/session/repair/motor identity.

Новые/задействованные audit-модули:

```text
CM_BackupBusinessDataIntegrityAudit.*
CM_MaterialPersistenceIntegrityAudit.*
CM_WindingPersistenceIntegrityAudit.*
CM_WarehousePersistenceIntegrityAudit.*
CM_WarehouseMovementIntegrityAudit.*
CM_PersistentIdIntegrityAudit.*
CM_ConductorSettingsIntegrityAudit.*
CM_WindingSessionPersistenceIntegrityAudit.*
```

Backup UI умеет объяснять причины instability для этих доменов.

## 8. Последняя серия backup-коммитов этого чата

```text
1a21073f  Add backup business data integrity audit contract
b9236557  Implement backup business data integrity audit
5a15dbfa  Audit workshop and pricing in backup manifest
0b68f3ce  Explain business data backup instability on mobile
f6cdd6d7  Explain business data backup instability on desktop
11191769  Add winding persistence integrity audit contract
1406c73f  Implement winding persistence integrity audit
fe988944  Audit winding persistence only when backup is safe
bbd0a507  Explain winding backup instability on mobile
21d5ab1b  Explain winding backup instability on desktop
b8ee44ce  Audit warehouse persistence in backup manifest
80958209  Explain warehouse persistence backup instability on mobile
e0d19ebe  Explain warehouse persistence backup instability on desktop
70220e54  Audit persistent allocator state in backup manifest
92a6a11e  Add conductor settings integrity audit contract
8f5cc608  Implement conductor settings integrity audit
fe683d95  Complete deep backup persistence audit
d4e194c9  Explain complete backup integrity failures on mobile
c3e2cdab  Explain complete backup integrity failures on desktop
```

## 9. Последняя найденная CI-ошибка и исправление

Пользователь дал Actions run:

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

`WString.h [-Wconversion]` в том же логе — warnings, не причина failure.

Исправлено минимально: добавлена отсутствующая closing brace namespace `CM` в конце `CM_MaterialLedger.cpp`.

Коммит исправления:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8  Fix MaterialLedger namespace closure
```

Важно: этот handoff фиксирует сам fix, но не утверждает, что новый ESP32 Actions run уже GREEN, пока это не подтверждено фактическим run/result.

## 10. Что было запланировано непосредственно перед переносом

### A. Cleanup winding backup audit

`CM_WindingPersistenceIntegrityAudit` стоит упростить так, чтобы он использовал authoritative:

```text
WindingJournalQuery::validateAll()
```

вместо обхода pagination/cursor для полного файла. Цель — меньше временных JSON buffers и меньше дублирования parser logic.

Перед правкой заново fetch:

```text
firmware/esp32/src/CM_WindingPersistenceIntegrityAudit.cpp
firmware/esp32/src/CM_WindingJournalQuery.h
firmware/esp32/src/CM_WindingJournalQueryValidation.cpp
```

### B. Проверить ESP32 build после `77fd7dd4`

Не ждать вручную без причины, но при следующем доступном Actions run сначала проверить реальную первую ошибку, если build снова красный.

Если пользователь даёт run URL/ID:

1. `fetch_workflow_run_jobs`;
2. failed `build-esp32` job id;
3. `fetch_workflow_job_logs`;
4. исправлять первую реальную `error:`/linker failure, не хвост framework warnings.

### C. Audit новых backup HTTP/error semantics

Проверить, что operator/API semantics остаются последовательными:

```text
storage unavailable -> 503
integrity/read failure -> 500
active winding / unsafe heavy export -> 409 или documented blocked state
not found -> 404
bad request -> 400
```

Особенно проверить `snapshot_stability_checked`, nullable `snapshot_stable`, `blocked_reason`, session directory errors и новые instability reasons.

### D. Performance / NDJSON growth

Следующий repo-reviewable эксплуатационный блок после integrity:

- оценить многократные full scans в manifest/finalization/costing;
- не мигрировать преждевременно в новую БД;
- при необходимости добавить bounded indexes/rotation/summary snapshots без ослабления fail-closed semantics.

### E. Hardware E2E — обязательный внешний этап

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

### F. Deferred product work

Не начинать без явной потребности:

- analogue/unassigned winding production model;
- automatic material writeoff;
- automatic safe resume;
- database migration.

## 11. Рекомендуемый порядок чтения в новом чате

1. `docs/PROJECT_HANDOFF/00_READ_FIRST.md`
2. этот файл `12_LATEST_HANDOFF_2026-08-08.md`
3. `01_CURRENT_STATE.md`
4. `06_ACTIVE_WORK_AND_NEXT_STEPS.md`
5. актуальные исходники конкретного следующего изменения
6. `09_KEY_FILES_INDEX.md`
7. `08_WORK_RULES_AND_VERIFICATION.md`
8. остальные handoff-документы при необходимости

`11_FULL_BRANCH_AUDIT.md` считать исторической картой, не текущим source of truth.

## 12. Точная точка продолжения

После handoff:

1. Не повторять уже сделанный backup-integrity block.
2. Проверить/упростить `CM_WindingPersistenceIntegrityAudit` через `validateAll()`.
3. При наличии нового ESP32 Actions failure — читать полный job log и чинить первую реальную ошибку.
4. Затем audit HTTP/error semantics backup.
5. После этого — performance/rotation review либо hardware E2E, в зависимости от доступности стенда.
