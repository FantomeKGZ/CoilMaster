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

Для больших `n` это может стать `O(n²)` или `O(n*m)` по числу записей. До реальных размеров это не повод менять storage engine, но это первый кандидат на оптимизацию.

### 3. Winding journal

Deep winding audit сейчас правильно разделён на две read-only проверки:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

`validateAll()` идёт до EOF без cursor pagination и без построения временных JSON history pages. Transition audit отдельно проверяет state-machine semantics.

Не объединять их преждевременно только ради одного прохода: distinct validators дают более простой fail-closed reasoning. Оптимизировать общий scan стоит только после измерений.

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

## Этап 1 — убрать повторные сканы внутри одного request

При подтверждённой необходимости:

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

После текущего repository audit — не переписывать storage. На hardware E2E и эксплуатационном стенде собрать реальные размеры/latency, затем выбрать один hotspot и оптимизировать его измеримо.

Hardware E2E ESP32 + Arduino остаётся обязательным отдельным подтверждением и этим документом не считается выполненным.
