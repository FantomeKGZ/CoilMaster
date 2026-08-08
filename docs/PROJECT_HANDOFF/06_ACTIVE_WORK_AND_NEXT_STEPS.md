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
- deep backup persistence integrity audits;
- winding backup `validateAll()` cleanup;
- backup/run-level HTTP semantics audit.

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
winding_journal_record_count=null
warehouse_movement_record_count=null
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

`validateAll()` идёт по journal до EOF и не строит temporary JSON history pages. Поэтому дополнительный pagination cleanup не нужен.

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
CM_WarehouseMovementIntegrityAudit.*
```

Результат repository review:

- публичные audit headers имеют собственный `FS.h` для `fs::FS`;
- count overloads winding/warehouse movement audit имеют explicit `<stdint.h>` для `uint32_t`;
- старые no-arg `check(storage)` контракты сохранены и делегируют новые overloads;
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

## Performance/rotation review — strategy готова, Stage 0 observability реализована

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

Manifest теперь имеет три runtime metrics без дополнительного persistence I/O:

```text
snapshot_stability_duration_ms
winding_journal_record_count
warehouse_movement_record_count
```

`winding_journal_record_count` считается внутри authoritative `WindingJournalQuery::validateAll()` прохода и становится доступен только после успешной schema validation + transition audit. Если winding integrity не доказана, поле `null`.

`warehouse_movement_record_count` считается внутри уже выполняемого `WarehouseMovementIntegrityAudit::check()` прохода. Это число непустых persisted rows (`PENDING` и завершающие `CONFIRMED|ABORTED`); отдельного повторного чтения `/data/warehouse/movements.ndjson` ради метрики нет. Поле `null`, если файл отсутствует, audit не запускался/не был достигнут или integrity не доказана.

Новые commits текущего performance/observability batch:

```text
8b61f46e1cb9d866bf9aa94800dd6a95f347c6b0  Measure deep backup audit duration
4696f5385c3749b2a824615033c1b29b284e79b5  Document backup audit timing semantics
59d208a8f86e803308824c43c2c3894df500f69d  Record backup audit observability baseline
3b1dbcc37ba00218e564b45edb42ace7105f2566  Record backup audit timing in current state
28945a079520169f450fe70a312a40c7b4d598e7  Advance active work to backup observability
c35b87717f7b64178f7c942f0228bd301771a78e  Expose winding journal validation count
36e0aee29506be33608f42bb2d7bfca87713b280  Count records during winding journal validation
eeea77a35e0938692f2142b5022ea41849bf5f64  Expose winding audit record count
1101ab18ef6a39e087e5f3b62814ec5d584b871c  Return validated winding record count
a1aa70381f53d10578fbb483a1335a96c8818551  Expose winding journal count in backup manifest
b84da0162ba73492742a261807c645eb1263b44b  Make winding audit count type explicit
cdf3c665d536d42e6638970d5a3694be03b014ce  Document winding journal backup metric
495823c09b8c61213767d05966ca4434f185aef9  Document winding journal observability
89f86252b94445d25b8bc1e5f1979595243407e4  Record winding backup observability in current state
63614fe363adaf912fdf35775ecff6befad34ed6  Expose warehouse movement audit count
fe024d6908e4488e114b633c97d06848d2d9bc38  Count warehouse movement audit records
a78cf149dd5d1f588988ddda3e2d046459fd36b5  Expose warehouse movement audit count
08743e86987f28ae051a13f6d30e79869eff07ff  Document warehouse movement observability
f6c98a6a132709d5be880d30714f1218207685ae  Fix winding observability commit reference
e98600ebdbd343de6a7260a4019b32924f55f742  Record warehouse backup observability
```

Не делать unbounded RAM mirror NDJSON на ESP32. Rotation обязана сохранять provenance/global IDs, fail-closed cross-segment validation, recoverable marker protocol и полноту backup whitelist.

## Следующее repo-reviewable действие

До аппаратного стенда **не вводить rotation trigger и не менять storage format**. Текущий Stage 0 уже даёт достаточно, чтобы начать реальные измерения двух append-only backup hotspots:

```text
winding-events.size_bytes
winding_journal_record_count
warehouse-movements.size_bytes
warehouse_movement_record_count
snapshot_stability_duration_ms
```

Следующий repo-only код допустим только если ещё один authoritative validator может вернуть count/duration в том же уже выполняемом проходе без дополнительного чтения файла. Наиболее естественный следующий кандидат при продолжении без стенда — material usage/adjustment audit, но только через совместимый overload и без второго scan ради телеметрии.

После hardware measurements выбрать один hotspot по фактическому размеру/latency. Только тогда решать, нужен ли bounded in-request index, rotation или read-only summary.

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

На backup этапе дополнительно снять и сохранить:

```text
winding-events.size_bytes
winding_journal_record_count
warehouse-movements.size_bytes
warehouse_movement_record_count
snapshot_stability_duration_ms
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
