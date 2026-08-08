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
- отдельно отмечать backup/finalization/costing scan.

Не хранить optimistic cache как доказательство integrity после mutation/reboot.

### Уже начато

Manifest теперь возвращает двадцать runtime metrics:

```text
snapshot_stability_duration_ms
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

Семантика `snapshot_stability_duration_ms`:

- при `snapshot_stability_checked=true` — длительность фактически выполненного deep audit;
- при `snapshot_stability_checked=false` — `null`;
- измерение оборачивает уже существующий `snapshotStabilityReason()` и **не добавляет дополнительный filesystem scan**;
- поле только observability metadata и не влияет на `snapshot_stable`/`export_allowed`.

Семантика `winding_allocator_last_id`:

- берётся из уже выполняемого `PersistentIdIntegrityAudit` чтения `id-state.txt`;
- соответствует validated `last_job_id == last_session_id` и является high-water mark выделенных winding job/session IDs;
- старый `PersistentIdIntegrityAudit::check(storage)` сохранён и делегирует совместимый metrics overload;
- при отсутствующем `/data/winding-jobs` successful pristine audit возвращает `0`;
- при failed allocator audit поле наружу не публикуется;
- дополнительного чтения allocator state ради этой метрики нет.

Семантика material counters:

```text
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
```

- считаются внутри уже существующих scans `materials.ndjson`, `usage.ndjson`, `adjustments.ndjson`;
- учитываются только непустые rows, прошедшие соответствующий parser/reference/arithmetic validation;
- старый `MaterialPersistenceIntegrityAudit::check(storage)` сохранён и делегирует overload с metrics;
- metrics публикуются только после полного успешного текущего `MaterialPersistenceIntegrityAudit::check()`, включая его существующие dependency audits;
- если deep audit не запускался, до material audit не дошли или material persistence не доказана — поля `null`;
- если audit успешен, но конкретный material файл отсутствует, его count равен `0`; наличие файла отдельно видно через `items[].exists`;
- отдельного повторного чтения material NDJSON ради counters нет.

Семантика business/workshop counters:

```text
workshop_client_record_count
workshop_motor_record_count
workshop_repair_record_count
repair_status_record_count
repair_pricing_record_count
```

- считаются внутри уже существующих validation passes `BackupBusinessDataIntegrityAudit` по `clients.ndjson`, `motors.ndjson`, `repairs.ndjson`, `repair-status.ndjson` и `pricing.ndjson`;
- `client/motor/repair` counts собираются в существующем uniqueness pass, status/pricing counts — в их текущих parser/reference passes;
- старый `BackupBusinessDataIntegrityAudit::check(storage)` сохранён и делегирует совместимый metrics overload;
- counts публикуются только после полного успешного business audit; partial counts при failure наружу не выдаются;
- если соответствующий файл отсутствует и audit в целом успешен, count равен `0`, а наличие отдельно видно через `items[].exists`;
- дополнительные full-file scans ради этих counters не добавлены.

Семантика `winding_journal_record_count`:

- считается внутри того же `WindingJournalQuery::validateAll()` прохода до EOF;
- публикуется только после успешной schema validation **и** `WindingJournalTransitionAudit::validate()`;
- если deep audit не запускался, до winding audit не дошли или winding integrity не доказана — `null`;
- отдельного повторного чтения `/data/winding-runs/events.ndjson` ради count нет.

Семантика session file counters и byte totals:

```text
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
winding_snapshot_total_bytes
winding_state_total_bytes
winding_spool_selection_total_bytes
```

- file counts считаются внутри уже существующих deep parser/cross-identity проходов `WindingSessionPersistenceIntegrityAudit` по snapshot/state/spool-selection directories;
- byte totals суммируются из `entry.size()` в тех же directory passes до штатного parser/load; дополнительного directory/full-file scan ради bytes нет;
- старый `WindingSessionPersistenceIntegrityAudit::check(storage)` сохранён и делегирует совместимый metrics overload;
- file counts публикуются только после полного успешного session persistence audit; partial counts при failure наружу не выдаются;
- если соответствующая directory отсутствует и audit в целом успешен, её count и total bytes равны `0`;
- byte totals не влияют на integrity result: если 32-bit сумма размера не представима, session audit продолжает обычную fail-closed validation, а только три total-byte metrics становятся `null`;
- штатные snapshot/state/spool-selection parser и cross-file identity checks не заменены телеметрией.

Семантика warehouse persistence counters:

```text
warehouse_spool_record_count
warehouse_price_record_count
```

- считаются внутри уже существующих `WarehousePersistenceIntegrityAudit` проходов по `/data/warehouse/spools.ndjson` и `/data/warehouse/price.ndjson`;
- spool count увеличивается только после полной проверки строки, включая canonical monotonic `spool_id`, diameter/weight/status/wire type и присутствующие optional string fields;
- price count увеличивается только после успешной проверки persisted price row;
- старый `WarehousePersistenceIntegrityAudit::check(storage)` сохранён и делегирует совместимый metrics overload;
- оба counts публикуются только после полного успешного warehouse persistence audit, включая существующий movement-reference pass; partial counts при failure наружу не выдаются;
- отсутствующий spool/price файл при успешном audit даёт `0`, наличие файла отдельно видно через `items[].exists`;
- дополнительных full-file scans ради этих counters нет.

Семантика `warehouse_movement_record_count`:

- считается внутри уже выполняемого `WarehouseMovementIntegrityAudit::check()` прохода по `/data/warehouse/movements.ndjson`;
- это число непустых NDJSON-records, включая transaction rows `PENDING` и завершающие `CONFIRMED|ABORTED`;
- публикуется только если файл существует и warehouse movement audit завершился успешно;
- если deep audit не запускался, до warehouse movement audit не дошли, файл отсутствует или integrity не доказана — `null`;
- старый `WarehouseMovementIntegrityAudit::check(storage)` сохранён и делегирует overload с count;
- отдельного повторного чтения `movements.ndjson` ради count нет.

Ключевые commits:

```text
8b61f46e1cb9d866bf9aa94800dd6a95f347c6b0  Measure deep backup audit duration
c35b87717f7b64178f7c942f0228bd301771a78e  Expose winding journal validation count
36e0aee29506be33608f42bb2d7bfca87713b280  Count records during winding journal validation
1101ab18ef6a39e087e5f3b62814ec5d584b871c  Return validated winding record count
a1aa70381f53d10578fbb483a1335a96c8818551  Expose winding journal count in backup manifest
b84da0162ba73492742a261807c645eb1263b44b  Make winding audit count type explicit
63614fe363adaf912fdf35775ecff6befad34ed6  Expose warehouse movement audit count
fe024d6908e4488e114b633c97d06848d2d9bc38  Count warehouse movement audit records
a78cf149dd5d1f588988ddda3e2d046459fd36b5  Expose warehouse movement audit count
ac031d8cc14a74786e10c8adb782776b0d16e97f  Expose material persistence audit counts
6cf4ad7da157c8e65f131b9a851c4243c0914e31  Count material persistence audit records
38befe338cfc57879d2ad09fc6be20a084c6ba2b  Expose material audit counts in backup manifest
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
```

Теперь на стенде можно сопоставить как минимум:

```text
workshop-clients.size_bytes
workshop_client_record_count
workshop-motors.size_bytes
workshop_motor_record_count
workshop-repairs.size_bytes
workshop_repair_record_count
repair-status.size_bytes
repair_status_record_count
repair-pricing.size_bytes
repair_pricing_record_count
materials.size_bytes
material_catalog_record_count
material-usage.size_bytes
material_usage_record_count
material-adjustments.size_bytes
material_adjustment_record_count
winding-events.size_bytes
winding_journal_record_count
winding_allocator_last_id
winding_snapshot_file_count
winding_snapshot_total_bytes
winding_state_file_count
winding_state_total_bytes
winding_spool_selection_file_count
winding_spool_selection_total_bytes
warehouse-spools.size_bytes
warehouse_spool_record_count
warehouse-price.size_bytes
warehouse_price_record_count
warehouse-movements.size_bytes
warehouse_movement_record_count
snapshot_stability_duration_ms
```

Это даёт сравнимые данные по растущим business/material/winding/warehouse persistence paths, allocator high-water и session-file population/bytes без изменения storage format и без дополнительного audit I/O.

## Этап 1 — убрать повторные сканы внутри одного request

При подтверждённой необходимости:

- первым проверить влияние transitive `MaterialPersistenceIntegrityAudit → WorkshopPersistenceIntegrityAudit` повторных scans;
- отдельно сопоставить рост business reference scans с `workshop_*_record_count` и `repair_pricing_record_count`, потому что текущий business audit намеренно использует повторные uniqueness/reference lookups;
- отдельно сопоставить warehouse reference-scan стоимость с `warehouse_spool_record_count` и `warehouse_movement_record_count`;
- сопоставить `winding_allocator_last_id` с session file counts/bytes: большой разрыв сам по себе допустим из-за legacy/archive semantics, но полезен как сигнал накопления/истории;
- если влияние заметно, разделить локальную material-file validation и cross-domain dependency validation так, чтобы backup orchestration выполнял каждый authoritative domain audit один раз;
- старые public contracts для других callers сохранять совместимыми, пока не доказано обратное;
- переиспользовать authoritative validators вместо повторного JSON/page parsing;
- строить bounded in-memory ID index только на время одного audit/request;
- размер index должен иметь явный верхний предел и fail-closed результат при превышении;
- использовать monotonic/sorted ID assumptions только там, где writer contract это гарантирует;
- не создавать unbounded RAM mirror всего NDJSON на ESP32;
- не ослаблять проверку corrupted references ради скорости.

Это предпочтительнее миграции storage engine.

## Этап 2 — bounded rotation immutable append histories

После измерения реальных размеров рассматривать rotation прежде всего для append-only history/log файлов, например:

```text
/data/winding-runs/events.ndjson
/data/warehouse/movements.ndjson
/data/materials/usage.ndjson
/data/materials/adjustments.ndjson
/data/repairs/pricing.ndjson
```

Перед реализацией для каждого файла отдельно подтвердить writer semantics и immutable-history invariant.

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
- rotation должна иметь atomic/recoverable marker protocol;
- incomplete rotation после reboot должна определяться fail-closed и восстанавливаться явно;
- backup whitelist/manifest должен перечислять/экспортировать все необходимые segments, иначе backup перестанет быть полным.

Session snapshot/state/spool-selection уже разделены по session files и не требуют того же rotation-механизма.

## Этап 3 — summary snapshots только для чтения

Для reports/dashboard можно добавить versioned read-only summary snapshots, если пересчёт полной истории станет заметно дорогим.

Summary не должен становиться единственным источником для:

- mutation authorization;
- CLOSED/finalization integrity;
- duplicate run/write-off prevention;
- backup integrity proof.

При сомнении authoritative history перечитывается fail-closed.

## Когда рассматривать БД

Только после фактических измерений, если одновременно выполняются несколько условий:

- bounded rotation всё ещё даёт неприемлемую latency;
- RAM-bounded per-request indexes недостаточны;
- transactional cross-entity queries стали доминирующей нагрузкой;
- recovery/backup semantics можно сохранить или улучшить;
- миграция имеет проверяемый rollback/compatibility plan.

До этого SQLite/другая БД добавит migration/recovery complexity без доказанной необходимости.

## Следующее практическое действие

На hardware E2E/эксплуатационном стенде снять реальные `size_bytes`, business/material/winding/warehouse record counts, allocator high-water, session file counts/byte totals и `snapshot_stability_duration_ms`, затем выбрать hotspot по измерению. До этого не вводить rotation trigger и не строить постоянный cache.

Отдельно сравнить общую длительность deep audit с business/material/warehouse counts и количеством/объёмом session files: текущий material audit транзитивно запускает broad workshop/winding/warehouse checks, business audit использует повторные reference/uniqueness scans, а warehouse movement-reference audit повторно ищет spool/repair references.

Следующий repo-only шаг до стенда допустим только если ещё один authoritative validator может вернуть count/duration/high-water/size **в том же существующем проходе** и это не расширяет hot path дополнительным I/O. Не начинать Stage 1 refactor только ради эстетики до benchmark, если нет отдельной correctness причины.

Hardware E2E ESP32 + Arduino остаётся обязательным отдельным подтверждением и этим документом не считается выполненным.
