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
- отдельно отмечать backup/finalization/costing scan.

Не хранить optimistic cache как доказательство integrity после mutation/reboot.

### Уже начато

Manifest теперь возвращает девять runtime metrics:

```text
snapshot_stability_duration_ms
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
winding_journal_record_count
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
warehouse_movement_record_count
```

Семантика `snapshot_stability_duration_ms`:

- при `snapshot_stability_checked=true` — длительность фактически выполненного deep audit;
- при `snapshot_stability_checked=false` — `null`;
- измерение оборачивает уже существующий `snapshotStabilityReason()` и **не добавляет дополнительный filesystem scan**;
- поле только observability metadata и не влияет на `snapshot_stable`/`export_allowed`.

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

Семантика `winding_journal_record_count`:

- считается внутри того же `WindingJournalQuery::validateAll()` прохода до EOF;
- публикуется только после успешной schema validation **и** `WindingJournalTransitionAudit::validate()`;
- если deep audit не запускался, до winding audit не дошли или winding integrity не доказана — `null`;
- отдельного повторного чтения `/data/winding-runs/events.ndjson` ради count нет.

Семантика session file counters:

```text
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
```

- считаются внутри уже существующих deep parser/cross-identity проходов `WindingSessionPersistenceIntegrityAudit` по snapshot/state/spool-selection directories;
- старый `WindingSessionPersistenceIntegrityAudit::check(storage)` сохранён и делегирует совместимый metrics overload;
- учитываются только файлы, которые прошли canonical filename check, штатный parser/load и требуемые cross-file identity checks;
- counts публикуются только после полного успешного session persistence audit; partial counts при failure наружу не выдаются;
- если соответствующая directory отсутствует и audit в целом успешен, её count равен `0`;
- отдельного directory/full-file scan ради этих counters не добавлено.

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
38befe338cfc57879d2ad09fc6be54d54c190441  Expose material audit counts in backup manifest
9c33178d8b580460e1d34962322fe81b9771dccc  Expose winding session persistence counts
afd2c9e3df2e63b59553e4f10e12eb4d2199e46d  Count winding session persistence files
abc4b02ef284ed86fdfc3e31149ccf8adf9d5e8b  Expose winding session file counts in backup manifest
```

Теперь на стенде можно сопоставить как минимум:

```text
materials.size_bytes
material_catalog_record_count
material-usage.size_bytes
material_usage_record_count
material-adjustments.size_bytes
material_adjustment_record_count
winding-events.size_bytes
winding_journal_record_count
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
warehouse-movements.size_bytes
warehouse_movement_record_count
snapshot_stability_duration_ms
```

Это даёт сравнимые данные по нескольким растущим persistence paths и session-file population без изменения storage format и без дополнительного audit I/O.

## Этап 1 — убрать повторные сканы внутри одного request

При подтверждённой необходимости:

- первым проверить влияние transitive `MaterialPersistenceIntegrityAudit → WorkshopPersistenceIntegrityAudit` повторных scans;
- если оно заметно, разделить локальную material-file validation и cross-domain dependency validation так, чтобы backup orchestration выполнял каждый authoritative domain audit один раз;
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

На hardware E2E/эксплуатационном стенде снять реальные `size_bytes`, material/winding/warehouse record counts, session file counts и `snapshot_stability_duration_ms`, затем выбрать hotspot по измерению. До этого не вводить rotation trigger и не строить постоянный cache.

Отдельно сравнить общую длительность deep audit с material history counts и количеством session files: текущий material audit транзитивно запускает broad workshop/winding/warehouse checks, поэтому рост material histories и накопление session files могут сочетаться с уже существующим повторным cross-domain I/O.

Следующий repo-only шаг до стенда допустим только если ещё один authoritative validator может вернуть count/duration **в том же существующем проходе** и это не расширяет hot path дополнительным I/O. Не начинать Stage 1 refactor только ради эстетики до benchmark, если нет отдельной correctness причины.

Hardware E2E ESP32 + Arduino остаётся обязательным отдельным подтверждением и этим документом не считается выполненным.
