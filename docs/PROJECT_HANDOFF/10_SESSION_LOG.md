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

Документационные коммиты до этой записи:

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
