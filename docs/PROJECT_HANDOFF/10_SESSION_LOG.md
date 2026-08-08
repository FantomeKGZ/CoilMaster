# Журнал рабочих сессий

Этот файл обновляется после каждой рабочей сессии.

## Формат записи

```text
## YYYY-MM-DD HH:MM — Название

Цель:

Что сделано:

Файлы:

Коммиты:

Проверка:

Где остановились:

Следующее действие:
```

---

## 2026-08-08 21:00 — Flat JSON runtime persistence hardening

Цель:

После завершения 29-metric Stage 0 перейти только к подтверждённым correctness-проблемам: исключить ситуацию, когда syntactically malformed flat NDJSON с ещё читаемыми отдельными полями проходит startup/deep/runtime reader и влияет на API, costing, warehouse summary или manual writeoff preflight.

Что сделано:

- добавлен общий header-only `CM_FlatJsonObjectValidator.h` без внешней JSON dependency и без дополнительного SD pass;
- deep business/pricing/material/warehouse/conductor settings authoritative passes теперь проверяют полный flat JSON syntax;
- `RepairRegistry` fail-closed проверяет workshop JSON также в runtime reads/lookups;
- strict parser выполняется один раз на authoritative outer pass; из O(n²)/O(n*m) nested identity/reference scans повторный parser убран/не добавлен;
- `RepairCosting::load()` проверяет movement/material-usage/pricing rows перед расчётом operator-visible totals;
- material adjustment history, usage history, pricing history и warehouse writeoff history reject malformed JSON до формирования API response/aggregates;
- `MaterialLedger::adjustMaterial()` и pending adjustment recovery теперь требуют valid flat JSON для source material, pending metadata/audit row, durable adjustment rows и direct state lookups; rewritten material row проверяется до temp-file write;
- `MaterialLedger::loadActiveMaterialCurrency()` теперь fail-closed на malformed catalog row;
- warehouse summary, movement summary и next spool ID проверяют flat JSON в текущих runtime passes;
- manual writeoff `nextMovementId()` и `rewriteSpoolWeight()` проверяют persisted state до PENDING transaction/spool swap; rewritten spool row проверяется до temp-file write;
- `loadWarehousePrice()` и exact `loadActiveSpoolIdentity()` fail-closed проверяют persisted JSON непосредственно в writeoff/job preflight paths;
- physical START, SSR ownership, reboot policy, PENDING→CONFIRMED|ABORTED semantics и manual `spool_id + source_session_id + source_run_id` writeoff invariants не менялись;
- `docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md`, `01_CURRENT_STATE.md`, `06_ACTIVE_WORK_AND_NEXT_STEPS.md`, `09_KEY_FILES_INDEX.md`, `12_LATEST_HANDOFF_2026-08-08.md` уже синхронизированы с базовым flat-JSON hardening block; текущая запись фиксирует последующее расширение runtime consumers.

Ключевые commits базового JSON hardening:

```text
9ddabf613f1edf95dc1da55cbba8763414e47968  Add flat persisted JSON syntax validator
96c863b1a1bde3a3725596940e74805da2c69111  Reject malformed flat JSON in business backup audit
d13269bb481d056623569b9ecdf91708be6b0b8b  Fail closed on malformed workshop JSON
07b20e9b88012446fcbc813e78b39349ecd70753  Reject malformed flat JSON in pricing audit
b86794238cab420d1d97c3b281fa233b92ccf317  Avoid repeated JSON parsing in business identity scans
86b19f35cad6c383377dee9c342e58f4978b0e79  Avoid repeated JSON parsing in registry duplicate scans
16f39b33cccaeed533c39c2e0144d6942169b2c7  Reject malformed flat JSON in material persistence audit
b3fd050c5e917691877e1c245fadde333742eed7  Reject malformed flat JSON in warehouse persistence audit
b7b362bfe1813f27eab0c904dc9c7fc4489e6f9e  Reject malformed flat JSON in movement audit
ab0b1f6b0381641e811fed5a18ac412ebb0667d2  Reject malformed flat JSON in conductor settings audit
090acf40fc4470c7b8719975df7f4ce218a3cdec  Keep pricing reference scans identity focused
02a80ea88a157ebfaee389d9c87368b36235ccbd  Fail closed on malformed costing histories
```

Ключевые commits последующего runtime/history/writeoff hardening:

```text
4d1761aa7b2b958a880b7de2b30c3bc8b09e62cd  Reject malformed pricing history JSON
9fb8918eb8fad0c939cf04330ddfce81073c1f8c  Reject malformed write-off history JSON
b43568c4231d9c972ed92f0b3b23c038837e3252  Reject malformed material usage history JSON
8c3f2b76464918a889a0e5a42d27c6ac4ff82c63  Fail closed on malformed warehouse summary state
d5c0a99449132fd343e1dc04e92e4136471dfbbb  Fail closed on malformed write-off transaction state
ea37009b15cf5367d4c5712bdd412c16f4649827  Fail closed on malformed warehouse price state
e01b7f8e6219558622a1c86c517c4d200f767992  Fail closed on malformed spool identity state
```

Также в этом блоке отдельными commits усилены:

```text
firmware/esp32/src/CM_MaterialHistory.cpp
firmware/esp32/src/CM_MaterialAdjustment.cpp
firmware/esp32/src/CM_MaterialLedgerCurrency.cpp
```

Проверка:

Static repository review подтверждает self-contained `CM_FlatJsonObjectValidator.h` через `Arduino.h`, отсутствие новой library dependency и отсутствие нового filesystem pass. Generated mutation JSON/formulas/provenance не менялись. GREEN CI для текущего head не подтверждён; hardware E2E не выполнялся.

Важно: большой `CM_MaterialLedger.cpp` был отдельно fetched и подготовлен к аналогичному hardening, но GitHub connector safety-filter остановил большой `update_file` **до записи**. Обход safety-filter через low-level Git object commit намеренно не использовался. Поэтому `CM_MaterialLedger.cpp` не считать полностью hardened этим блоком; его direct catalog/usage/rewrite readers остаются конкретным repo-only кандидатом.

Где остановились:

Flat JSON correctness hardening закрывает deep audits, workshop runtime, costing, material adjustment/currency/history consumers и основные warehouse summary/manual-writeoff consumers. Stage 0 metrics не расширялись дальше: 29 полей достаточно для hardware benchmark.

Следующее действие:

1. Если connector позволяет штатный SHA-guarded update без safety block — вернуться к `CM_MaterialLedger.cpp` и заменить только подтверждённые `{...}`/direct persisted row shortcuts на shared validator без новых scans.
2. Продолжить audit оставшихся реально найденных split runtime readers/recovery helpers, не угадывая архитектуру и не меняя safety semantics.
3. Обязательный внешний этап: ESP32 + Arduino E2E и один `/api/backup/manifest` с `items[].size_bytes` + всеми 29 metrics.
4. Stage 1 duplicate-scan/rotation/database work не начинать до benchmark, если нет отдельной correctness причины.

---

## 2026-08-08 20:04 — Per-domain deep backup timing

Цель:

Довести Stage 0 observability до прямого измерения latency каждого уже выполняемого deep-backup domain без дополнительного SD I/O и без Stage 1 refactor до benchmark.

Что сделано:

- в `CM_BackupExportWeb.cpp` добавлен внутренний `AuditTimingMetric`;
- через `millis()` измеряются уже существующие вызовы allocator, conductor settings, material persistence, business data, winding persistence, warehouse persistence, warehouse movements и winding session persistence audits;
- отдельно измеряется preliminary session-directory scan перед authoritative deep session audit;
- manifest получил 9 новых per-domain duration fields;
- если deep audit не запускался, timing fields `null`;
- если конкретный domain выполнялся и завершился failure, его duration сохраняется, а последующие неисполненные domains остаются `null`;
- timing не меняет порядок audit, fail-closed result или safety gating;
- Stage 0 observability теперь содержит 29 runtime metrics;
- синхронизированы `docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md`, `01_CURRENT_STATE.md`, `05_COMPLETED_WORK_LOG.md`, `06_ACTIVE_WORK_AND_NEXT_STEPS.md`, `09_KEY_FILES_INDEX.md`, `12_LATEST_HANDOFF_2026-08-08.md`.

Новые timing fields:

```text
persistent_id_audit_duration_ms
conductor_settings_audit_duration_ms
material_persistence_audit_duration_ms
business_data_audit_duration_ms
winding_persistence_audit_duration_ms
warehouse_persistence_audit_duration_ms
warehouse_movements_audit_duration_ms
winding_session_directory_scan_duration_ms
winding_session_persistence_audit_duration_ms
```

Кодовый commit:

```text
96a1c5bc8c4a5cb7f5b672d290bbac23867429c5  Measure deep backup domain durations
```

Документационные commits этой части:

```text
60ad1619c87f822b39a8f2da325bf287aeb21a30  Document per-domain backup audit timings
403ce7b391d8db455092bc9190ef348bcc3d4258  Record per-domain backup audit timings
7b3281d52bccda189c6c495f30c9a82d1d9d579c  Advance active work to per-domain timings
9a151ddf13cfcafa3f7f8756781710fdb4107d80  Refresh latest handoff per-domain timings
9a4ba295e9cd12481163557a3603d489eac96bd4  Index per-domain backup audit timings
0ff202c41973adc0269e175ddaa15cc600e19a9b  Update completed work per-domain timings
```

Проверка:

Static repository review подтвердил, что `CM_BackupExportWeb.h` явно включает `Arduino.h`, timing использует `uint32_t` wrap-safe subtraction `millis() - startedAtMs`, и telemetry не добавляет filesystem scan. GREEN CI текущего head не подтверждён. Hardware E2E не выполнялся.

Где остановились:

Stage 0 теперь достаточно подробен для реального benchmark: total audit duration + 9 domain durations + population/high-water/session bytes. Известные duplicate-I/O/reference-scan hotspots не рефакторить до измерений без отдельной correctness-причины.

Следующее действие:

1. Реальный ESP32 + Arduino E2E production path.
2. Сохранить один backup manifest с `items[].size_bytes` и всеми 29 Stage 0 metrics.
3. Выбрать самый дорогой `*_duration_ms`, затем сопоставить его с counts/bytes/high-water.
4. Только после этого решать bounded index / audit decomposition / rotation.

---

## 2026-08-08 19:40 — Allocator high-water и winding session byte observability

Цель:

Продолжить Stage 0 repository-only observability без дополнительного full-file I/O, без изменения storage model и без ослабления safety-инвариантов.

Что сделано:

- подтверждено, что warehouse spool/price counters уже появились в актуальной ветке параллельной работой, поэтому этот блок не дублировался;
- `PersistentIdIntegrityAudit` расширен совместимым `PersistentIdIntegrityAuditMetrics` overload;
- authoritative allocator audit теперь возвращает validated `lastAllocatedId` из уже читаемого `id-state.txt`, где `last_job_id == last_session_id`;
- backup manifest публикует `winding_allocator_last_id`; дополнительного чтения allocator state ради telemetry нет;
- `WindingSessionPersistenceAuditMetrics` расширен `snapshotTotalBytes`, `stateTotalBytes`, `spoolSelectionTotalBytes` и флагом `byteTotalsAvailable`;
- суммарные session bytes считаются через `entry.size()` в уже выполняемых snapshot/state/spool-selection directory passes до штатного parser/load;
- дополнительного directory/full-file scan ради byte totals не добавлено;
- telemetry overflow 32-bit суммы не считается persistence corruption: в таком случае только byte totals становятся недоступны, а authoritative session integrity audit продолжает прежнюю fail-closed validation;
- manifest публикует `winding_snapshot_total_bytes`, `winding_state_total_bytes`, `winding_spool_selection_total_bytes` только после полного успешного session audit и при доступной 32-bit сумме;
- Stage 0 observability доведён до 20 runtime metrics: duration, allocator high-water, material/business/winding/warehouse counts, session counts и session total bytes;
- обновлены `docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md`, `01_CURRENT_STATE.md`, `06_ACTIVE_WORK_AND_NEXT_STEPS.md`, `09_KEY_FILES_INDEX.md`, `12_LATEST_HANDOFF_2026-08-08.md`.

Файлы:

```text
firmware/esp32/src/CM_PersistentIdIntegrityAudit.h
firmware/esp32/src/CM_PersistentIdIntegrityAudit.cpp
firmware/esp32/src/CM_WindingSessionPersistenceIntegrityAudit.h
firmware/esp32/src/CM_WindingSessionPersistenceIntegrityAudit.cpp
firmware/esp32/src/CM_BackupExportWeb.cpp
docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/09_KEY_FILES_INDEX.md
docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md
```

Кодовые коммиты:

```text
4a30e4ca08e1d2e010dded1ab3e93073f9ecaeed  Expose persistent allocator audit metrics
b38bb3b5190bb99d261f5552cecedfea4048289b  Return validated allocator high-water mark
52fae7716034ccacebc41f1f11715f5eebf193c2  Expose allocator high-water mark in backup manifest
1470b866c0b91aee4bd8dff1eddc6c26926be578  Expose winding session byte totals
cacdffa9ec822ad1425d6a4de34c10f836fbbab0  Measure winding session persistence bytes
a0c83b08f64c05f0232d287146850f9e9fd37ce5  Expose winding session byte totals in backup manifest
```

Документационные commits до этой записи:

```text
19f026e778675a5423fa446e6595bc1115547f85  Document allocator and session byte observability
5d39cef6b454c21aac37b2a80c360dcd81e6d615  Record allocator and session byte metrics
ba8f7c89d5f23f0549608b7603f6503d4fd6d514  Advance active work observability metrics
599610f1786f7e68cd45c6da40ee7acf9b8093fd  Index allocator and session byte metrics
3352ef0e00b79aedd9b7df9224bd0a1075c42625  Refresh latest handoff Stage 0 metrics
```

Проверка:

Static repository review подтверждает совместимость overloads/includes, отсутствие нового telemetry-only scan и сохранение `BackupActivityGuard::Safe` gating. Telemetry byte overflow отделён от integrity failure. Это не заменяет фактический ESP32 build. GREEN CI для текущего head в этой записи не подтверждён. Hardware E2E не выполнялся.

Где остановились:

Stage 0 теперь даёт достаточно данных для первого реального benchmark: размер статических NDJSON, record counts, allocator high-water, session population/bytes и общую длительность deep audit. Известные duplicate-I/O/reference-scan hotspots остаются Stage 1 кандидатами только после измерений или при отдельной correctness-причине.

Следующее действие:

1. Реальный ESP32 + Arduino E2E production path.
2. Одновременно сохранить один backup manifest с `size_bytes`, всеми 20 Stage 0 metrics и `snapshot_stability_duration_ms`.
3. По измеренной latency/size/population выбрать первый hotspot для bounded index / audit decomposition / rotation.
4. Если доступен новый Actions run — проверить фактический `build-esp32` head и исправлять только подтверждённую compile/link ошибку.

---

## 2026-08-08 19:03 — Business backup observability и handoff sync

Цель:

Продолжить от фактического HEAD `cmp-protocol-v1` после winding cleanup, не повторять уже завершённые блоки и взять следующий repo-reviewable Stage 0 шаг без дополнительного full-file I/O.

Что сделано:

- прочитаны все актуальные файлы `docs/PROJECT_HANDOFF/` и подтверждено, что `CM_WindingPersistenceIntegrityAudit` cleanup через `WindingJournalQuery::validateAll()` уже завершён;
- подтверждено, что HTTP/error semantics audit также уже закрыт, поэтому работа продолжена от фактической точки ветки;
- выбран same-pass observability для business/workshop persistence;
- `BackupBusinessDataAuditMetrics` добавляет counts клиентов, двигателей, ремонтов, repair-status и pricing;
- старый `BackupBusinessDataIntegrityAudit::check(storage)` сохранён и делегирует metrics overload;
- client/motor/repair counts собираются внутри уже существующих uniqueness passes, repair-status/pricing counts — внутри текущих parser/reference passes;
- partial business metrics не публикуются при failed audit;
- backup manifest публикует `workshop_client_record_count`, `workshop_motor_record_count`, `workshop_repair_record_count`, `repair_status_record_count`, `repair_pricing_record_count` только после полного успешного business audit;
- при первой попытке обновить `CM_BackupExportWeb.cpp` GitHub вернул `409`, потому что файл изменился параллельно; файл был заново fetched из `cmp-protocol-v1`, после чего изменения наложены поверх актуального blob без потери новых winding session counters;
- сохранены параллельно появившиеся `winding_snapshot_file_count`, `winding_state_file_count`, `winding_spool_selection_file_count` и их `WindingSessionPersistenceAuditMetrics` contract;
- Stage 0 observability теперь содержит 14 runtime metrics и даёт пары `size_bytes + record_count` для workshop/pricing/material/winding/warehouse growth paths плюс session-file population и общую audit duration;
- обновлены `docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md`, `06_ACTIVE_WORK_AND_NEXT_STEPS.md`, `09_KEY_FILES_INDEX.md`, `12_LATEST_HANDOFF_2026-08-08.md`; `01_CURRENT_STATE.md` уже был синхронизирован параллельной работой и лишний commit не создавался.

Файлы:

```text
firmware/esp32/src/CM_BackupBusinessDataIntegrityAudit.h
firmware/esp32/src/CM_BackupBusinessDataIntegrityAudit.cpp
firmware/esp32/src/CM_BackupExportWeb.cpp
docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/09_KEY_FILES_INDEX.md
docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md
```

Кодовые коммиты:

```text
cf7df132d190bd359a4f4b85b2553f6dcdba5dd4  Expose business data audit counts
33746bf301ee8a31417361ce6fd8f7a2ce1635f7  Count business backup audit records
1871d140e1e493b6e64ada3502eaa5fbcb75f0f6  Expose business audit counts in backup manifest
```

Сохранённые параллельные winding-session observability commits:

```text
9c33178d8b580460e1d34962322fe81b9771dccc  Expose winding session persistence counts
afd2c9e3df2e63b59553e4f10e12eb4d2199e46d  Count winding session persistence files
abc4b02ef284ed86fdfc3e31149ccf8adf9d5e8b  Expose winding session file counts in backup manifest
```

Проверка:

Static repository review подтверждает совместимость overloads/includes, сохранение `BackupActivityGuard::Safe` gating и отсутствие нового telemetry-only full scan. Это не заменяет ESP32 build. GREEN CI текущего head в этой записи не подтверждён. Hardware E2E не выполнялся.

Где остановились:

Winding cleanup, deep backup integrity, HTTP semantics audit и Stage 0 observability для material/business/winding/warehouse + session files завершены на repository level. Известный duplicate-I/O composition path и business reference scans остаются измеряемыми Stage 1 кандидатами, но не должны рефакториться до benchmark без отдельной correctness причины.

Следующее действие:

1. Реальный ESP32 + Arduino E2E production path.
2. Одновременно снять manifest `size_bytes`, Stage 0 metrics и `snapshot_stability_duration_ms`.
3. По фактическим latency/size/population выбрать первый hotspot.

---

## 2026-08-08 18:50 — Winding cleanup verification и material backup observability

Цель:

Продолжить с обязательного cleanup `CM_WindingPersistenceIntegrityAudit`, затем перейти к следующему repo-reviewable улучшению без изменения safety-инвариантов.

Что сделано:

- подтвержден authoritative `WindingJournalQuery::validateAll(recordCount)` + отдельный transition audit;
- cursor-pagination full scan в backup отсутствует;
- `CM_WindingSessionPersistenceIntegrityAudit` не дублирован;
- material same-pass counts и manifest integration подтверждены;
- transitive duplicate-I/O path зафиксирован как measured Stage 1 candidate.

Проверка:

Repository-level static review, не GREEN CI и не hardware E2E.

---

## 2026-08-08 12:35 — Deep backup integrity, Actions diagnosis и handoff

Deep backup persistence integrity собран. Actions failure `31243187630` был вызван missing closing brace в `CM_MaterialLedger.cpp`; fix `77fd7dd4`. Hardware E2E остаётся обязательным.

---

## 2026-08-07 16:11 — Обновление документации для переноса

Backend full winding flow и основные safety/integrity слои были собраны; handoff синхронизирован.

---

## 2026-08-06 — Усиление журнала намотки

Добавлены strict `RUN_STARTED/RUN_COMPLETED` sequencing, monotonic run IDs и one-active-run semantics.

---

## 2026-08-06 — Создание постоянного handoff-контекста

Создан `docs/PROJECT_HANDOFF/` и корневой continuation entrypoint.
