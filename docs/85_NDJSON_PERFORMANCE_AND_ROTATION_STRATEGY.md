# NDJSON performance и rotation strategy

Дата: 2026-08-08  
Ветка: `cmp-protocol-v1`

## Решение

Сейчас **не мигрировать в БД**. Текущая microSD + NDJSON архитектура остаётся production baseline, пока измерения не покажут, что bounded streaming/rotation недостаточны.

Repository review ниже определяет hotspots и безопасный порядок оптимизации. Это не hardware benchmark.

## Найденные hotspots

### 1. Deep backup manifest

При `BackupActivityGuard::Safe` manifest последовательно проверяет практически весь persistent whitelist и session contents. Это правильно для fail-closed backup, но стоимость растёт вместе с файлами.

### 2. Cross-file integrity lookups

Некоторые текущие integrity audits намеренно простые и надёжные, но выполняют повторные сканы:

- business audit проверяет uniqueness/reference через повторное открытие NDJSON;
- material usage/adjustments повторно ищут referenced material/repair;
- warehouse movements повторно ищут spool/repair references.

Дополнительно repository review подтвердил важный composition hotspot: текущий `MaterialPersistenceIntegrityAudit::check()` после собственных materials/usage/adjustments scans транзитивно вызывает `WorkshopPersistenceIntegrityAudit::check()` и `RepairPricingIntegrityAudit::check()`. `WorkshopPersistenceIntegrityAudit` в свою очередь снова проверяет warehouse, allocator, session persistence и полный winding journal + transition audit. В backup manifest эти domains затем отдельно проверяются ещё раз. Это не corruption bug, но это доказанный источник повторного I/O и кандидат Stage 1 после измерения влияния.

Для больших `n` повторные reference scans могут стать `O(n²)` или `O(n*m)` по числу записей. До реальных размеров это не повод менять storage engine.

### 3. Winding journal

Deep winding audit сейчас правильно разделён на две read-only проверки:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

`validateAll()` идёт до EOF без cursor pagination и без построения временных JSON history pages. Transition audit отдельно проверяет state-machine semantics.

Не объединять их преждевременно только ради одного прохода: distinct validators дают более простой fail-closed reasoning. Оптимизировать общий scan стоит только после измерений или при устранении уже доказанного transitive duplicate audit без потери checks.

### 4. Costing/finalization/history

Costing, finalization preflight и operator histories также читают append-only persisted histories. Их частота выше, чем у backup, поэтому при росте данных именно request latency может стать первым видимым ограничением.

## Этап 0 — измерить до оптимизации

Добавлять low-cost observability без изменения source of truth:

- file size bytes;
- record count во время уже выполняемого scan;
- scan duration/high-water mark;
- число файлов session directories;
- суммарный размер session directories во время уже выполняемого directory pass;
- длительность каждого уже выполняемого deep-audit domain;
- отдельно отмечать backup/finalization/costing scan.

Не хранить optimistic cache как доказательство integrity после mutation/reboot.

### Реализовано

Manifest теперь возвращает **29 runtime metrics** без отдельного telemetry full scan.

Общие и per-domain duration metrics:

```text
snapshot_stability_duration_ms
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

Population/high-water/size metrics:

```text
winding_allocator_last_id
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
workshop_client_record_count
workshop_motor_record_count
workshop_repair_record_count
repair_status_record_count
repair_pricing_record_count
winding_journal_record_count
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
winding_snapshot_total_bytes
winding_state_total_bytes
winding_spool_selection_total_bytes
warehouse_spool_record_count
warehouse_price_record_count
warehouse_movement_record_count
```

### Duration semantics

Все per-domain duration fields измеряются вокруг **уже существующего** вызова audit через `millis()` и wrap-safe unsigned subtraction. Дополнительного filesystem I/O ради timing нет.

Если deep audit вообще не запускался из-за machine activity, duration fields равны `null`. Если audit дошёл до конкретного domain, его duration публикуется даже когда этот domain завершился failure; последующие неисполненные domains остаются `null`. Это позволяет отличить slow failure от domain, который вообще не запускался.

`winding_session_directory_scan_duration_ms` отдельно измеряет уже существующий предварительный обход snapshot/spool-selection/state directories. `winding_session_persistence_audit_duration_ms` измеряет последующий authoritative deep parser/cross-identity audit. Это специально оставлено двумя полями, чтобы benchmark показал цену текущего предварительного directory scan до любого Stage 1 refactor.

Duration metrics являются observability metadata и не влияют на `snapshot_stable`, `export_allowed`, порядок audit или safety behavior.

### `snapshot_stability_duration_ms`

- при `snapshot_stability_checked=true` — длительность полного фактически выполненного `snapshotStabilityReason()` до success или первого failure;
- при `snapshot_stability_checked=false` — `null`;
- поле не добавляет filesystem scan.

### `winding_allocator_last_id`

- берётся из уже выполняемого `PersistentIdIntegrityAudit` чтения `id-state.txt`;
- соответствует validated `last_job_id == last_session_id` и является high-water mark выделенных winding job/session IDs;
- старый `PersistentIdIntegrityAudit::check(storage)` сохранён и делегирует совместимый metrics overload;
- при отсутствующем `/data/winding-jobs` successful pristine audit возвращает `0`;
- при failed allocator audit high-water не публикуется;
- дополнительного чтения allocator state ради метрики нет.

### Material counters

```text
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
```

- считаются внутри существующих scans `materials.ndjson`, `usage.ndjson`, `adjustments.ndjson`;
- учитываются только непустые rows, прошедшие соответствующий parser/reference/arithmetic validation;
- старый `MaterialPersistenceIntegrityAudit::check(storage)` сохранён и делегирует overload с metrics;
- counts публикуются только после полного successful material persistence audit;
- отсутствующий файл при successful audit даёт `0`, наличие отдельно видно через `items[].exists`;
- отдельного чтения файлов ради counters нет.

### Business/workshop counters

```text
workshop_client_record_count
workshop_motor_record_count
workshop_repair_record_count
repair_status_record_count
repair_pricing_record_count
```

- считаются внутри существующих `BackupBusinessDataIntegrityAudit` validation passes;
- client/motor/repair counts собираются в uniqueness passes, status/pricing — в parser/reference passes;
- старый `check(storage)` сохранён через metrics overload;
- partial counts при failure наружу не публикуются;
- дополнительные full-file scans ради counters не добавлены.

### Winding journal counter

`winding_journal_record_count` считается внутри того же authoritative `WindingJournalQuery::validateAll()` прохода до EOF и публикуется только после успешной schema + transition validation. Дополнительного чтения журнала ради count нет.

### Session file counters и byte totals

```text
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
winding_snapshot_total_bytes
winding_state_total_bytes
winding_spool_selection_total_bytes
```

- file counts считаются в существующих deep parser/cross-identity passes `WindingSessionPersistenceIntegrityAudit`;
- byte totals суммируются из `entry.size()` в тех же directory passes до штатного parser/load;
- старый `check(storage)` сохранён и делегирует metrics overload;
- counts/bytes публикуются только после полного successful session audit;
- отсутствующая directory при successful audit даёт `0`;
- если 32-bit aggregate byte sum не представима, только три total-byte metrics становятся `null`; integrity audit продолжает обычную fail-closed validation;
- дополнительного directory/full-file scan ради byte totals нет.

### Warehouse persistence counters

```text
warehouse_spool_record_count
warehouse_price_record_count
```

- считаются внутри уже существующих `WarehousePersistenceIntegrityAudit` spool/price passes;
- публикуются только после полного successful warehouse persistence audit, включая movement-reference validation;
- старый `check(storage)` сохранён через metrics overload;
- дополнительных full-file scans ради counters нет.

### Warehouse movement counter

`warehouse_movement_record_count` считается внутри существующего `WarehouseMovementIntegrityAudit::check()` прохода по `/data/warehouse/movements.ndjson`. Это число непустых transaction rows, включая `PENDING` и завершающие `CONFIRMED|ABORTED`; отдельного чтения ради counter нет.

## Flat persisted JSON correctness hardening

После завершения Stage 0 repository review выявил отдельную correctness-причину, не связанную с benchmark: часть flat NDJSON validators считала строку структурно допустимой по внешним `{...}` и выборочному извлечению полей. Синтаксически повреждённая строка с ещё читаемым ID могла пройти часть startup/deep checks и затем попасть в read-only API как malformed JSON.

Добавлен header-only `CM_FlatJsonObjectValidator.h` без внешней JSON dependency. Он проверяет полный синтаксис одного flat JSON object в уже прочитанной `String`: string keys, primitive string/number/bool/null values, whitespace и JSON escapes. Nested object/array намеренно не принимаются, потому что затронутые persisted schemas flat.

Shared validator теперь применяется на authoritative outer passes для:

```text
workshop clients/motors/repairs/repair-status
repair pricing
materials/material usage/material adjustments
warehouse spools/price/movements
conductor settings
```

`RepairRegistry` также проверяет flat JSON в runtime reads/lookups, поэтому corruption после boot не должен превращаться в malformed JSON API output.

Performance invariant сохранён: strict JSON syntax проверяется один раз на authoritative outer pass. В уже известных O(n²)/O(n*m) duplicate/reference scans parser намеренно не дублируется; такие scans остаются identity-focused. Это correctness hardening без дополнительного filesystem I/O и без преждевременного Stage 1 refactor.

Safety semantics physical START/SSR/manual wire writeoff не затронуты.

## Ключевые Stage 0 commits

```text
8b61f46e1cb9d866bf9aa94800dd6a95f347c6b0  Measure deep backup audit duration
c35b87717f7b64178f7c942f0228bd301771a78e  Expose winding journal validation count
36e0aee29506be33608f42bb2d7bfca87713b280  Count records during winding journal validation
1101ab18ef6a39e087e5f3b62814ec5d584b871c  Return validated winding record count
a1aa70381f53d10578fbb483a1335a96c8818551  Expose winding journal count in backup manifest
63614fe363adaf912fdf35775ecff6befad34ed6  Expose warehouse movement audit count
fe024d6908e4488e114b633c97d06848d2d9bc38  Count warehouse movement audit records
a78cf149dd5d1f588988ddda3e2d046459fd36b5  Expose warehouse movement audit count
ac031d8cc14a74786e10c8adb782776b0d16e97f  Expose material persistence audit counts
6cf4ad7da157c8e65f131b9a851c4243c0914e31  Count material persistence audit records
38befe338cfc57879d2ad09fc6be54d54c190441  Expose material audit counts in backup manifest
9c33178d8b580460e1d34962322fe81b9771dccc  Expose winding session persistence counts
afd2c9e3df2e63b59553e4f10e12eb4d2199e46d  Count winding session persistence files
abc4b02ef284ed86fdfc3e31149ccf8adf9d5e8b  Expose winding session file counts in backup manifest
cf7df132d190bd359a4f4b85b2553f6dcdba5dd4  Expose business data audit counts
33746bf301ee8a31417361ce6fd8f7a2ce1635f7  Count business backup audit records
1871d140e1e493b6e64ada3502eaa5fbcb75f0f6  Expose business audit counts in backup manifest
fdb2428895004b7f248504bab8ef85334651535a  Expose warehouse persistence audit counts
490bee61c81f6425bde5623818fdf80abaa089dc  Count warehouse persistence audit records
34645b743e5638d379287947a8a62ec61ea4ee90  Expose warehouse persistence counts in backup manifest
4a30e4ca08e1d2e010dded1ab3e93073f9ecaeed  Expose persistent allocator audit metrics
b38bb3b5190bb99d261f5552cecedfea4048289b  Return validated allocator high-water mark
52fae7716034ccacebc41f1f11715f5eebf193c2  Expose allocator high-water mark in backup manifest
1470b866c0b91aee4bd8dff1eddc6c26926be578  Expose winding session byte totals
cacdffa9ec822ad1425d6a4de34c10f836fbbab0  Measure winding session persistence bytes
a0c83b08f64c05f0232d287146850f9e9fd37ce5  Expose winding session byte totals in backup manifest
96a1c5bc8c4a5cb7f5b672d290bbac23867429c5  Measure deep backup domain durations
```

Ключевые correctness commits после Stage 0:

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
```

## Что снять на стенде

Сохранять один manifest вместе с `items[].size_bytes` и следующими полями:

```text
snapshot_stability_duration_ms
persistent_id_audit_duration_ms
conductor_settings_audit_duration_ms
material_persistence_audit_duration_ms
business_data_audit_duration_ms
winding_persistence_audit_duration_ms
warehouse_persistence_audit_duration_ms
warehouse_movements_audit_duration_ms
winding_session_directory_scan_duration_ms
winding_session_persistence_audit_duration_ms
winding_allocator_last_id
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
workshop_client_record_count
workshop_motor_record_count
workshop_repair_record_count
repair_status_record_count
repair_pricing_record_count
winding_journal_record_count
winding_snapshot_file_count
winding_snapshot_total_bytes
winding_state_file_count
winding_state_total_bytes
winding_spool_selection_file_count
winding_spool_selection_total_bytes
warehouse_spool_record_count
warehouse_price_record_count
warehouse_movement_record_count
```

Per-domain durations позволяют выбирать hotspot напрямую, а counts/bytes/high-water помогают объяснить, почему он растёт.

## Этап 1 — убрать повторные сканы внутри одного request

Только после измерений или отдельной correctness-причины:

- первым проверить `material_persistence_audit_duration_ms`, потому что `MaterialPersistenceIntegrityAudit → WorkshopPersistenceIntegrityAudit` транзитивно повторяет broad cross-domain checks;
- сравнить `business_data_audit_duration_ms` с workshop/pricing counts из-за повторных uniqueness/reference lookups;
- сравнить `warehouse_persistence_audit_duration_ms` и `warehouse_movements_audit_duration_ms` с spool/movement population;
- сравнить `winding_session_directory_scan_duration_ms` и `winding_session_persistence_audit_duration_ms`; preliminary directory pass и deep session audit намеренно пока не объединять до измерения;
- при доказанной необходимости разделить local material validation и cross-domain dependency validation так, чтобы backup orchestration выполнял каждый authoritative domain audit один раз;
- bounded in-memory ID indexes строить только на время одного request, с явным RAM limit и fail-closed поведением;
- не создавать unbounded RAM mirror NDJSON;
- не ослаблять corrupted-reference validation ради скорости.

## Этап 2 — bounded rotation immutable append histories

После измерения реальных размеров рассматривать rotation прежде всего для append-only history/log файлов:

```text
/data/winding-runs/events.ndjson
/data/warehouse/movements.ndjson
/data/materials/usage.ndjson
/data/materials/adjustments.ndjson
/data/repairs/pricing.ndjson
```

Рекомендуемая модель:

```text
closed immutable segment(s)
+ one active append segment
```

Rotation trigger выбирать по измеренному размеру/числу записей, а не по произвольной дате.

Обязательные invariants:

- global IDs/provenance не меняются при rotation;
- history API читает segments как один логический журнал;
- integrity audit проверяет все segments и их порядок;
- mutation не считается успешной, если active segment не сохранён;
- rotation имеет atomic/recoverable marker protocol;
- incomplete rotation после reboot определяется fail-closed;
- backup manifest/export включает все необходимые segments.

Session snapshot/state/spool-selection уже разделены по session files и не требуют того же rotation-механизма.

## Этап 3 — summary snapshots только для чтения

Для reports/dashboard можно добавить versioned read-only summary snapshots, если пересчёт полной истории станет заметно дорогим.

Summary не должен становиться единственным источником для mutation authorization, CLOSED/finalization integrity, duplicate prevention или backup integrity proof.

## Когда рассматривать БД

Только после фактических измерений, если bounded rotation и RAM-bounded per-request indexes остаются недостаточными, transactional cross-entity queries доминируют, а migration/recovery имеет проверяемый rollback/compatibility plan.

До этого SQLite/другая БД добавит complexity без доказанной необходимости.

## Следующее практическое действие

На hardware E2E/эксплуатационном стенде снять один полный manifest с 29 Stage 0 metrics и `items[].size_bytes`, затем выбрать hotspot по фактической latency/size/population.

До этого не вводить rotation trigger, persistent optimistic cache, database migration или Stage 1 duplicate-scan refactor только ради эстетики.

Hardware E2E ESP32 + Arduino остаётся обязательным отдельным подтверждением и этим документом не считается выполненным.
