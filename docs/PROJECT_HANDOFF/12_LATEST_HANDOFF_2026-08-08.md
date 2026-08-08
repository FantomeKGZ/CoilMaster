# CoilMaster — полный handoff на 2026-08-08

Ветка: `cmp-protocol-v1`  
Репозиторий: `FantomeKGZ/CoilMaster`  
Этот файл — актуальная точка входа после deep backup-integrity, HTTP semantics audit и первого Stage 0 performance/observability batch.

## 1. Обязательные правила продолжения

- Source of truth — только текущий код `cmp-protocol-v1`.
- `main` не использовать как источник кода или архитектурных решений.
- Перед каждым изменением существующего файла заново fetch его из `cmp-protocol-v1` и использовать текущий blob SHA.
- При `update_file` отправлять полный файл.
- Новый файл перед `create_file` проверять на отсутствие точного пути.
- Один и тот же файл не редактировать параллельно.
- Новые commits не считать GREEN без фактического Actions result/подтверждения пользователя.
- Hardware E2E не считать выполненным без реального стенда ESP32 + Arduino и подтверждения пользователя.

Safety invariants не менять:

- physical START только аппаратный;
- ESP32/WEB не управляют SSR напрямую;
- после reboot нет auto-resume;
- `RUN_COMPLETED` не делает automatic wire writeoff;
- ручное списание провода обязательно;
- CLOSED repair запрещает новые mutations;
- corruption/storage failure всегда fail-closed.

## 2. Production workflow уже собран

```text
client
→ motor с authoritative coil_program
→ repair OPEN
→ costing
→ linked winding
→ exact spool_id
→ immutable job snapshot + immutable spool selection
→ UART delivery
→ JOB_ACK ACCEPTED
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ winding history
→ manual wire writeoff по фактическому весу
→ source_session_id + source_run_id provenance
→ materials / pricing
→ finalization preflight
→ CLOSED
→ archive / monthly report
→ read-only backup/export
```

Не начинать этот workflow заново.

## 3. Winding persistence и exact spool

Реализованы:

- persistent `job_id/session_id` allocator;
- immutable job snapshot;
- immutable exact spool-selection;
- persisted runtime state;
- reboot recovery + manual review acknowledgement;
- strict repair/motor linkage;
- authoritative motor `coil_program`;
- единый `CM_WindingProgramParser`;
- winding journal schema 2;
- cursor pagination только для пользовательского read-only history API;
- semantic transition audit `RUN_STARTED → RUN_COMPLETED`;
- dynamic microSD readiness.

Persistence:

```text
/data/winding-jobs/id-state.txt
/data/winding-jobs/id-state.bak
/data/winding-jobs/snapshots/session-<id>.json
/data/winding-jobs/spool-selection/session-<id>.json
/data/winding-jobs/state/session-<id>.json
/data/winding-runs/events.ndjson
```

Важно: cleanup winding backup audit уже выполнен. `CM_WindingPersistenceIntegrityAudit` использует:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

Старого cursor-pagination полного scan там нет. Не возвращаться к этому пункту.

## 4. Manual wire writeoff provenance

Для новых linked runs writeoff имеет:

```text
source_session_id + source_run_id
```

Server-side доказывается:

- конкретный run имеет `RUN_COMPLETED`;
- immutable spool-selection относится к тому же repair;
- используется та же бухта;
- duplicate CONFIRMED для `(session_id, run_id)` запрещён;
- recovery сохраняет provenance через `PENDING → CONFIRMED | ABORTED`;
- finalization требует ручного coverage каждого нового completed linked run.

HTTP gap с пустыми run-level полями закрыт:

```text
362fcb7daa8f883f57de4867c06c42f06e45b613  Reject empty run provenance fields
```

Если `source_session_id` или `source_run_id` присутствует, оба обязаны присутствовать, быть непустыми и canonical non-zero IDs; иначе `400` без записи.

## 5. CLOSED и finalization

`OPEN → CLOSED` реализован server-side.

CLOSED запрещает:

- новый linked winding;
- wire writeoff;
- material usage;
- pricing revision.

Перед закрытием backend fail-closed проверяет unfinished/recovery winding state, persisted dependencies, winding journal/transition semantics и ручной wire coverage. Preflight read-only; реальный close независимо повторяет проверки.

## 6. Warehouse / materials / costing

Warehouse:

- CU/AL + legacy UNKNOWN;
- active spool catalogue;
- recoverable spool-file swap;
- writeoff transaction `PENDING → CONFIRMED | ABORTED`;
- startup recovery;
- strict movement parsing/references;
- exact spool identity + run provenance.

Materials:

- catalogue;
- usage/adjustment histories;
- pending recovery;
- recoverable swap;
- strict arithmetic/reference validation.

Costing:

- warehouse/material/pricing integrity;
- checked overflow;
- currency/formula validation;
- consistent `NEAREST_MINOR_UNIT` rounding.

## 7. Deep read-only backup/export

Endpoints:

```text
GET /api/backup/manifest
GET /api/backup/file?name=...
GET /api/backup/sessions
GET /api/backup/session-file?kind=...&session_id=...
```

Arbitrary filesystem paths запрещены.

Deep scan выполняется только если `BackupActivityGuard` доказывает `Safe`.

Во время active/unsafe winding:

```text
export_allowed=false
snapshot_stability_checked=false
snapshot_stable=null
snapshot_stability_duration_ms=null
winding_journal_record_count=null
```

При safe state `snapshot_stable=true` означает успешную read-only проверку всего static whitelist:

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

И необходимых adjuncts/invariants:

- material/warehouse recovery and swap markers;
- allocator main/optional backup/absence of temp;
- conductor settings contents + absence of `.tmp/.bak` recovery residue;
- workshop/material/pricing/warehouse references + arithmetic;
- winding journal schema до EOF;
- winding transition semantics;
- canonical session directories;
- содержимое всех snapshot/state/spool-selection files;
- cross-file job/session/repair/motor/spool identity.

Основные audit modules:

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

## 8. HTTP/error semantics — закрыто

Документ:

```text
docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md
```

Карта:

```text
400 malformed / missing required input
404 requested allowed resource absent
409 syntactically valid request blocked by domain/machine state
500 persisted read/integrity failure
503 storage/service dependency unavailable
```

Manifest остаётся status endpoint: active winding выражается `200` + blocked state; direct heavy export возвращает `409`.

Не делать массовый refactor кодов без нового доказанного semantic gap.

## 9. NDJSON performance strategy — без преждевременной БД

Документ:

```text
docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
```

Порядок:

```text
Stage 0  измерить size / record count / latency
Stage 1  убрать доказанно дорогие repeated scans bounded per-request indexes
Stage 2  recoverable rotation immutable append histories при необходимости
Stage 3  read-only summary snapshots для reports/dashboard
DB       только если измерения докажут недостаточность предыдущих этапов
```

Нельзя:

- делать unbounded RAM mirror всех NDJSON;
- вводить persistent optimistic cache как доказательство integrity;
- выбирать rotation threshold без измерений;
- ослаблять fail-closed проверки ради скорости.

## 10. Stage 0 observability — уже реализована

Manifest теперь возвращает:

```text
snapshot_stability_duration_ms
winding_journal_record_count
```

`duration_ms` измеряет только уже выполняемый deep audit.

`winding_journal_record_count` считается внутри того же authoritative `WindingJournalQuery::validateAll()` EOF pass. Отдельного повторного чтения `events.ndjson` ради count нет.

Count публикуется только после успешной winding schema validation **и** transition audit. Если winding integrity не доказана — `null`.

Эти поля — observability metadata и не участвуют в `snapshot_stable` / `export_allowed`.

Кодовые commits Stage 0:

```text
8b61f46e1cb9d866bf9aa94800dd6a95f347c6b0  Measure deep backup audit duration
c35b87717f7b64178f7c942f0228bd301771a78e  Expose winding journal validation count
36e0aee29506be33608f42bb2d7bfca87713b280  Count records during winding journal validation
eeea77a35e0938692f2142b5022ea41849bf5f64  Expose winding audit record count
1101ab18ef6a39e087e5f3b62814ec5d584b871c  Return validated winding record count
a1aa70381f53d10578fbb483a1335a96c8818551  Expose winding journal count in backup manifest
b84da0162ba73492742a261807c645eb1263b44b  Make winding audit count type explicit
```

Документационные commits текущего batch:

```text
4696f5385c3749b2a824615033c1b29b284e79b5  Document backup audit timing semantics
59d208a8f86e803308824c43c2c3894df500f69d  Record backup audit observability baseline
3b1dbcc37ba00218e564b45edb42ace7105f2566  Record backup audit timing in current state
28945a079520169f450fe70a312a40c7b4d598e7  Advance active work to backup observability
cdf3c665d536d42e6638970d5a3694be03b014ce  Document winding journal backup metric
495823c09b8c61213767d05966ca4434f185aef9  Document winding journal observability
89f86252b94445d25b8bc1e5f1979595243407e4  Record winding backup observability in current state
17fc3883cb2eeeefc6ff79f289e0b6e3a87db663  Advance handoff to measured backup performance
```

Эти commits **не считать GREEN автоматически**.

## 11. Compile-safety status

Repository-level include audit выполнен.

Для нового winding count API:

- старый no-arg `validateAll()` сохранён и делегирует overload;
- старый `WindingPersistenceIntegrityAudit::check(storage)` сохранён;
- count overload имеет explicit `<stdint.h>` для публичного `uint32_t`;
- `CM_BackupExportWeb` уже имеет Arduino dependency для `millis()`.

Это статическая compile-safety проверка, не фактический build result.

Последняя ранее подтверждённая CI ошибка была missing namespace closing brace в `CM_MaterialLedger.cpp`; исправлена:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8  Fix MaterialLedger namespace closure
```

Не утверждать GREEN для текущего head без нового Actions run/result.

## 12. Hardware E2E — обязательный внешний этап

Happy path:

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

На backup этапе снять:

```text
winding-events.size_bytes
winding_journal_record_count
snapshot_stability_duration_ms
```

Fault scenarios минимум:

- microSD unavailable before job;
- runtime microSD loss;
- reboot after ACCEPTED before START;
- reboot after RUN_STARTED;
- corrupted snapshot/state/spool-selection/journal;
- corrupted warehouse/material/pricing/workshop persistence;
- dangling warehouse PENDING;
- material pending/swap markers;
- duplicate writeoff same `(session_id, run_id)`;
- close without manual wire coverage;
- backup during active winding;
- backup with temp/pending/corrupt state.

## 13. Точная точка продолжения

1. Не повторять deep backup integrity, winding `validateAll()` cleanup или HTTP semantics audit.
2. Если появляется новый Actions run — проверить фактический `build-esp32` head и исправлять первую реальную compile/link error, а не framework warnings.
3. Предпочтительный следующий шаг — реальный hardware/E2E + performance measurement с `size_bytes`, `winding_journal_record_count`, `snapshot_stability_duration_ms`.
4. Если hardware пока недоступен, repo-only код продолжать только как same-pass observability: новый counter/duration допустим, только если authoritative validator может вернуть его без второго full scan.
5. Не вводить rotation threshold, persistent cache или database migration до измерений.

## 14. Deferred product work

Не начинать автоматически:

- analogue/unassigned winding production model;
- automatic wire/material writeoff по `RUN_COMPLETED`;
- automatic safe resume;
- database migration;
- direct SSR control с ESP32/WEB.

## 15. Порядок чтения в следующем чате

```text
1. docs/PROJECT_HANDOFF/00_READ_FIRST.md
2. docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md
3. docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
4. docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
5. актуальные исходники конкретного изменения
6. docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md
7. docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
8. docs/PROJECT_HANDOFF/09_KEY_FILES_INDEX.md
9. docs/PROJECT_HANDOFF/08_WORK_RULES_AND_VERIFICATION.md
```

Код `cmp-protocol-v1` всегда выше документации по приоритету. `11_FULL_BRANCH_AUDIT.md` — историческая карта, не текущий source of truth.
