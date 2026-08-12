# Где остановились и что делать дальше

Дата обновления: **2026-08-12**  
Ветка: **`cmp-protocol-v1`**

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

Полный актуальный continuation checkpoint текущего чата сохранён в:

```text
23_CHAT_CONTINUATION_2026-08-12.md
```

## Hardware production status

На реальном стенде подтверждены:

```text
Standalone Arduino local program
→ physical START
→ real winding
→ LOCAL_EVT
→ ESP32 autonomous archive
```

и полный linked production flow:

```text
client / motor / OPEN repair
→ linked winding
→ exact spool
→ UART
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ manual exact-run wire writeoff
→ costing / finalization
→ CLOSED
→ final data found
→ stable backup readable
```

Happy-path hardware E2E закрыт.

Hardware negative checkpoint подтверждён:

```text
active RUN_STARTED
→ backup request
→ export blocked
→ deep stability scan not started
```

## Safety boundary — не менять

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся ручным и exact `spool_id + source_session_id + source_run_id`.

## Verification status текущего HEAD

Последний подтверждённый пользователем ESP32 build был до текущих archive/workshop scaling изменений:

```text
RAM:   14.4% (47320 / 327680 bytes)
Flash: 86.7% (1136237 / 1310720 bytes)
SUCCESS
```

Текущий HEAD изменён после него:

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

GitHub CI не считать green без фактического result.

## Закрытый production-hardening

Закрыты и задокументированы:

```text
14_HARDWARE_MANIFEST_BASELINE_2026-08-12.md
15_HARDWARE_PRODUCTION_E2E_2026-08-12.md
16_HARDWARE_NEGATIVE_BACKUP_ACTIVE_2026-08-12.md
17_RUNTIME_STORAGE_CORRUPTION_HARDENING_2026-08-12.md
18_UART_TIMEOUT_REPLAY_HARDENING_2026-08-12.md
19_CONTROLLED_RECOVERY_AND_ARCHIVE_SCALING_2026-08-12.md
20_AUTONOMOUS_ARCHIVE_PAGING_2026-08-12.md
21_AUTONOMOUS_ARCHIVE_BOOT_AND_APPEND_SCALING_2026-08-12.md
22_WORKSHOP_REGISTRY_SCALING_2026-08-12.md
23_CHAT_CONTINUATION_2026-08-12.md
```

Ключевые safety результаты:

```text
TIMED_OUT → MANUAL_REVIEW_REQUIRED
manual-review / ambiguous persisted state → backup blocked
linked recovery → exact snapshot + exact immutable spool-selection
wrong spool/session/run → writeoff rejected before warehouse mutation
RUN_COMPLETED alone → no automatic wire writeoff
```

## Autonomous Arduino archive — scaling state

Authoritative append-only files:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

### Deep/boot audit

```text
events       O(E)
assignments  fixed batches of 32 references
```

`AutonomousWindingArchive::begin()` использует тот же authoritative `validateStorage()`.

### HTTP/UI

```text
GET /api/autonomous-windings
limit default 20
limit max 32
cursor opaque byte offset
has_more
next_cursor
```

Cursor не разделяет normal `RUN_STARTED/RUN_COMPLETED` pair.

### Runtime append

`LOCAL_EVT` replay/completion использует bounded tail-state, а не full history scan.

Непустой NDJSON обязан заканчиваться `\n`; interrupted append fail-closed.

## Workshop registry — current scaling state

Authoritative files:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
/data/repairs/pricing.ndjson
```

### Bounded list readers are now mandatory/default

Для:

```text
GET /api/clients
GET /api/motors
GET /api/repairs
```

любой list request теперь bounded, даже если `cursor`/`limit` отсутствуют:

```text
cursor default 0
limit default 20
limit max 32
count
has_more
next_cursor
max_page_size
```

Legacy unbounded response mode удалён.

Ключевой commit:

```text
a6a136d0ed595825441b58faedae932dd89b1586
Make workshop list paging mandatory
```

Legacy formatter declarations и implementations удалены:

```text
e81f34c98600393b160f5f1b0f9c0234e3031498
Remove legacy unbounded registry formatters

a4282018b5deeaa989494ec46f57975f8f47edad
Remove legacy unbounded registry readers
```

Удалены:

```text
appendClientsJson()
appendMotorsJson()
appendRepairsJson()
```

`appendSimilarMotorsJson()` остаётся отдельным similarity API.

### Exact lookup

```text
GET /api/clients/by-id?client_id=...
GET /api/motors/by-id?motor_id=...
GET /api/repairs/by-id?repair_id=...
```

Exact lookup fail-closed отклоняет duplicate matching identity.

### Legacy consumers migrated

На exact/paged reads переведены:

```text
desktop costing
mobile + desktop linked winding
mobile + desktop reports
mobile + desktop wire writeoff lifecycle
mobile + desktop materials lifecycle
```

Reports агрегируют repair pages по 32 и exact lookup client/motor только для нужных ремонтов. Финансовый итог остаётся fail-closed.

Pricing-audit уже использовал scoped costing/history API и отдельной миграции не требовал.

### Business-data integrity / boot

Старый O(n^2) audit заменён authoritative bounded-complexity audit:

```text
client IDs      strict monotonic append-only pass
motor IDs       strict monotonic pass + coil_program validation
repair IDs      strict monotonic pass
repair refs     batches of 32
CLOSED status   batches of 32 + exact-one status occurrence
pricing refs    batches of 32
```

`RepairRegistry::begin()` использует:

```text
BackupBusinessDataIntegrityAudit::checkWorkshopRegistry()
```

`nextId()` проходит monotonic ID ledger одним строгим проходом.

## Текущая точка выполнения

Consumer migration и удаление legacy unbounded workshop readers завершены в repo.

Активная задача — compile-safety audit после этого cleanup:

1. сверить declarations/definitions между:
   - `CM_RepairRegistry.h`;
   - `CM_RepairRegistry.cpp`;
   - `CM_RepairRegistryPage.cpp`;
   - `CM_RepairRegistryLookup.cpp`;
   - `CM_RepairRegistrySimilarity.cpp`;
2. убедиться, что нет stale references на удалённые `appendClientsJson / appendMotorsJson / appendRepairsJson`;
3. проверить cursor semantics: default `0/20`, max `32`, record-boundary validation, корректный progress `has_more/next_cursor`, filters across pages;
4. после repo compile-safety — clean ESP32 build.

## Следующее действие

```powershell
pio run -e esp32 -t clean
pio run -e esp32
```

Build считать подтверждённым только после реального `SUCCESS` от пользователя.

После `SUCCESS`:

```powershell
pio run -e esp32 -t upload
```

Затем полностью заменить microSD `/web` актуальным repo `firmware/esp32/web`. Не делать overlay поверх старого `/web`.

Runtime smoke check после upload:

```text
/api/clients
/api/motors
/api/repairs
```

Без args каждый endpoint должен вернуть только bounded first page с page metadata.

Также проверить exact by-ID endpoints и основные mobile/desktop screens: clients, motors, repairs, costing, linked winding context, reports, writeoff, materials.

## Следующий performance review после успешного build/runtime

`appendRepairsPageJson()` уже ограничивает число repair rows максимум 32, но текущий `repairClosed()` может сканировать status ledger отдельно для каждой строки страницы. Это потенциальный growing-I/O path.

Оптимизировать его только после успешного build/runtime текущего batch, предпочтительно через bounded/batched status resolution для одной страницы, без преждевременной смены storage format.

После этого снять populated-dataset timings через `/api/backup/manifest`.

Segmentation/rotation threshold вводить только по фактическим latency/size metrics. DB migration/destructive compaction без доказанной необходимости не вводить.

Основная новая функциональность сейчас не приоритет. Проект находится в production-hardening/performance phase.
