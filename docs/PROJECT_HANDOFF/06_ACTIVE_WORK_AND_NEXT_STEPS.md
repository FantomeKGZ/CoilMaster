# Где остановились и что делать дальше

Дата обновления: 2026-08-08
Ветка: `cmp-protocol-v1`

Самый полный свежий snapshot находится в:

```text
docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md
```

Этот файл фиксирует именно активную очередь работ. Код ветки выше документации по приоритету.

## Что уже закрыто — не делать повторно

Основной production path уже собран:

```text
client
→ motor
→ repair OPEN
→ costing
→ linked winding
→ exact spool_id
→ immutable snapshot + spool-selection
→ UART delivery
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ winding history
→ manual wire writeoff
→ source_session_id + source_run_id
→ materials/pricing
→ finalization preflight
→ CLOSED
→ archive/report
→ read-only backup/export
```

Не начинать заново:

- persistent allocator;
- snapshot/state/recovery;
- exact spool selection;
- linked repair/motor validation;
- winding journal/history;
- repair CLOSED lifecycle;
- warehouse/material crash recovery;
- strict costing;
- run-level manual wire provenance;
- finalization wire coverage;
- reports;
- whitelist backup/export;
- deep backup persistence integrity audits.

## Последний подтверждённый CI факт

Ранее пользователь передал Actions run:

```text
31243187630
```

`build-esp32` job:

```text
93067378338
```

Run собирал commit:

```text
78ac24533f1157080bd2163990dbdb0b2577807c
```

Первая реальная compile error была:

```text
firmware/esp32/src/CM_MaterialLedger.cpp:705:1:
error: expected '}' at end of input
```

Причина: отсутствующая closing brace namespace `CM`. Framework `WString.h [-Wconversion]` были только warnings.

Исправлено:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8  Fix MaterialLedger namespace closure
```

Repository review ниже не заменяет фактический ESP32 Actions build. Не утверждать GREEN для новых commits без реального run/result.

## Backup integrity — проверено и закрыто

Deep `snapshot_stable` audit запускается только когда `BackupActivityGuard` возвращает `Safe`.

Во время active winding:

```text
export_allowed=false
snapshot_stability_checked=false
snapshot_stable=null
snapshot_stability_duration_ms=null
```

При safe state deep audit охватывает **весь static backup whitelist**:

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

И дополнительно проверяет необходимые adjuncts/invariants:

- material/warehouse pending/swap markers;
- allocator main/optional backup/absence of temp;
- conductor settings contents + отсутствие `.tmp/.bak` recovery residue;
- workshop/material/pricing/warehouse references;
- winding journal schema + transition semantics;
- canonical session directories;
- **содержимое allocator/session persistence**, включая snapshot/state/spool-selection;
- cross-file job/session/repair/motor/spool identity.

## Winding backup cleanup — уже выполнен в коде

Проверены актуальные:

```text
firmware/esp32/src/CM_WindingPersistenceIntegrityAudit.cpp
firmware/esp32/src/CM_WindingJournalQuery.h
firmware/esp32/src/CM_WindingJournalQueryValidation.cpp
```

Старого cursor-pagination полного scan **нет**. Текущий audit уже использует:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

`validateAll()` идёт по journal до EOF и не строит temporary JSON history pages. Поэтому дополнительный code churn в `CM_WindingPersistenceIntegrityAudit.cpp` не нужен.

## Compile-safety audit новых backup integrity modules — выполнен статически

Проверены include-зависимости новых audit modules, включая:

```text
CM_BackupBusinessDataIntegrityAudit.*
CM_MaterialPersistenceIntegrityAudit.*
CM_WarehousePersistenceIntegrityAudit.*
CM_PersistentIdIntegrityAudit.*
CM_ConductorSettingsIntegrityAudit.*
CM_WindingPersistenceIntegrityAudit.*
CM_WindingSessionPersistenceIntegrityAudit.*
```

Результат repository review:

- публичные audit headers имеют собственный `FS.h` для `fs::FS`;
- `.cpp`, использующие `String`, `File`, `isDigit`, имеют Arduino/own-header dependency chain;
- ESP32 PlatformIO config компилирует все `firmware/esp32/src/*.cpp`;
- явной missing-include / incomplete-type зависимости в проверенном наборе не найдено.

Это compile-safety review, а не доказательство успешной физической сборки/CI.

## HTTP/error semantics audit — выполнен

Документ:

```text
docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md
```

Проверенный контракт:

```text
400 malformed/missing request fields
404 requested allowed resource not found
409 valid request blocked by machine/domain state
500 persisted read/integrity failure
503 storage/service dependency unavailable
```

Manifest намеренно остаётся status endpoint: active winding выражается через `200` + `export_allowed=false`, а direct export endpoints возвращают `409`.

Найден и исправлен run-level semantic gap: пустые, но присутствующие `source_session_id` + `source_run_id` больше не могут молча перейти в legacy write-off path.

Commit:

```text
362fcb7daa8f883f57de4867c06c42f06e45b613  Reject empty run provenance fields
```

## Performance/rotation review — strategy готова, observability начата

Документ:

```text
docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
```

Решение: **без преждевременной миграции в БД**.

Главные hotspots repository review:

- full deep backup scans;
- repeated cross-file reference scans в business/material/warehouse audits;
- costing/finalization/history по растущим append-only данным;
- winding journal имеет отдельные full schema + semantic scans, но schema scan уже не создаёт pagination JSON.

Порядок оптимизации:

```text
1. измерить file size / record count / scan latency
2. убрать повторные scans внутри одного request bounded indexes
3. при доказанной необходимости — rotation immutable append histories
4. summary snapshots только для read/report paths
5. БД рассматривать только при измеренном недостатке этих мер
```

Первый runtime signal уже добавлен без дополнительного I/O:

```text
snapshot_stability_duration_ms
```

Он измеряет только уже выполняемый deep audit; если `snapshot_stability_checked=false`, значение `null`. Поле не участвует в safety/integrity решениях.

Новые commits текущего performance batch:

```text
8b61f46e1cb9d866bf9aa94800dd6a95f347c6b0  Measure deep backup audit duration
4696f5385c3749b2a824615033c1b29b284e79b5  Document backup audit timing semantics
59d208a8f86e803308824c43c2c3894df500f69d  Record backup audit observability baseline
3b1dbcc37ba00218e564b45edb42ace7105f2566  Record backup audit timing in current state
```

Не делать unbounded RAM mirror NDJSON на ESP32. Rotation обязана сохранять provenance/global IDs, fail-closed cross-segment validation, recoverable marker protocol и полноту backup whitelist.

## Следующее repo-reviewable действие

До аппаратного стенда следующий безопасный участок — **не вводить rotation trigger**, а продолжить Stage 0 observability только там, где метрика получается без второго прохода по файлу.

Приоритет:

```text
1. проверить, какие authoritative validators уже считают/могут вернуть record count в том же scan;
2. если API можно расширить без дублирующего I/O — добавить bounded scan counters для одного наиболее дорогого audit path;
3. не создавать persistent cache и не менять storage format;
4. после hardware measurements выбрать hotspot по size_bytes + duration, а не по предположению.
```

Если появляется новый ESP32 Actions failure:

1. получить jobs конкретного run;
2. найти failed `build-esp32` job;
3. прочитать полный job log;
4. исправлять первую реальную `error:` / `fatal error:` / linker failure;
5. не считать framework warnings причиной failure без доказательства.

## Hardware E2E — обязательный внешний этап

Repository review и CI не доказывают физическое поведение ESP32 + Arduino. Не отмечать этот этап выполненным, пока пользователь сам не подтвердит стенд.

Happy path:

```text
1. client + motor + repair
2. warehouse price + active CU/AL spool
3. linked winding + exact spool_id
4. JOB_ACK ACCEPTED
5. physical START
6. RUN_STARTED
7. RUN_COMPLETED
8. winding history + immutable spool identity
9. manual wire writeoff
10. source_session_id + source_run_id
11. costing
12. finalization preflight
13. CLOSED
14. archive + monthly report
15. stable backup manifest/export
```

Fault scenarios:

- microSD unavailable before job;
- runtime microSD loss;
- reboot after ACCEPTED before START;
- reboot after RUN_STARTED;
- corrupted snapshot/state/spool-selection/journal;
- corrupted warehouse/material/pricing/workshop data;
- dangling warehouse PENDING;
- material pending/swap marker;
- duplicate writeoff same `(session_id, run_id)`;
- close without manual wire coverage;
- backup during active winding;
- backup with temp/pending/corrupt state.

## Deferred — не начинать автоматически

- analogue/unassigned winding production mode;
- automatic writeoff только по `RUN_COMPLETED`;
- automatic safe resume;
- database migration;
- direct SSR control с ESP32/WEB.

## Правило переноса

Новый чат читает в таком порядке:

```text
00_READ_FIRST.md
12_LATEST_HANDOFF_2026-08-08.md
01_CURRENT_STATE.md
06_ACTIVE_WORK_AND_NEXT_STEPS.md
актуальные исходники
09_KEY_FILES_INDEX.md
08_WORK_RULES_AND_VERIFICATION.md
```

Код `cmp-protocol-v1` всегда выше документации по приоритету.
