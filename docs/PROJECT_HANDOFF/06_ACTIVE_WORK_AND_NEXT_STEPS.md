# Где остановились и что делать дальше

Дата обновления: **2026-08-12**  
Ветка: **`cmp-protocol-v1`**

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

## Главная точка продолжения

Полный финальный handoff текущего процесса, включая историю выполнения, safety-инварианты, hardware checkpoints, commits, archive/workshop scaling, build-status, точку остановки и готовый текст для нового чата, сохранён в:

```text
docs/PROJECT_HANDOFF/24_NEW_CHAT_HANDOFF_2026-08-12.md
```

Предыдущие подробные checkpoints не удалять; `24_NEW_CHAT_HANDOFF_2026-08-12.md` является текущей входной точкой и ссылается на них.

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

## Verification status текущего code batch

Последний подтверждённый пользователем ESP32 build был до текущих archive/workshop scaling изменений:

```text
RAM:   14.4% (примерно 47320 / 327680 bytes)
Flash: 86.7% (примерно 1.136 MB / 1.310 MB)
SUCCESS
```

После него код менялся. Поэтому для текущего code batch:

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

GitHub CI не считать green без фактического результата.

## Закрытый production-hardening

Подробно зафиксирован в:

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
23_CHAT_CONTINUATION_HANDOFF_2026-08-12.md
24_NEW_CHAT_HANDOFF_2026-08-12.md
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

Текущее состояние:

```text
GET /api/autonomous-windings
limit default 20
limit max 32
opaque byte-offset cursor
has_more / next_cursor
```

Cursor не разделяет normal `RUN_STARTED/RUN_COMPLETED` pair.

Boot/deep audit использует authoritative bounded-complexity validation. Normal `LOCAL_EVT` append/replay использует bounded tail-state вместо полного прохода по history. Непустой NDJSON обязан заканчиваться `\n`; interrupted append fail-closed.

## Workshop registry — current scaling state

Authoritative files:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
/data/repairs/pricing.ndjson
```

### Bounded list API теперь обязательный/default

Для:

```text
GET /api/clients
GET /api/motors
GET /api/repairs
```

любой list request bounded, даже если `cursor`/`limit` отсутствуют:

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

Ключевые commits этого cleanup зафиксированы в `23`/`24`, включая:

```text
a6a136d0ed595825441b58faedae932dd89b1586
Make workshop list paging mandatory

e81f34c98600393b160f5f1b0f9c0234e3031498
Remove legacy unbounded registry formatters

a4282018b5deeaa989494ec46f57975f8f47edad
Remove legacy unbounded registry readers
```

Удалены general full-list readers:

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
desktop + mobile costing
desktop + mobile linked winding
desktop + mobile reports
desktop + mobile wire writeoff lifecycle
desktop + mobile materials lifecycle
clients / motors / repairs catalogs
Arduino archive motor catalog
```

Reports агрегируют repair pages по 32 и exact lookup client/motor только для нужных ремонтов. Финансовый итог остаётся fail-closed.

Pricing-audit уже использовал scoped costing/history API и отдельной миграции не требовал.

### Business-data integrity / boot

Старый O(n²) audit заменён authoritative bounded-complexity audit:

```text
client IDs      strict monotonic append-only pass
motor IDs       strict monotonic pass + coil_program validation
repair IDs      strict monotonic pass
repair refs     batches of 32
CLOSED status   batches of 32 + exact-one status occurrence
pricing refs    batches of 32
```

`RepairRegistry::begin()` использует authoritative workshop/business audit.

`nextId()` проходит monotonic ID ledger одним строгим проходом.

## Текущая точка выполнения

Consumer migration и удаление legacy unbounded workshop readers завершены в repo.

Текущий batch после этих изменений ещё не подтверждён clean build.

Перед следующим performance-refactor сначала:

1. re-fetch current `cmp-protocol-v1`;
2. коротко проверить declarations/definitions и отсутствие stale references на удалённые readers;
3. выполнить clean ESP32 build;
4. при SUCCESS записать RAM/Flash;
5. прошить ESP32;
6. полностью заменить microSD `/web` актуальным `firmware/esp32/web`;
7. выполнить bounded/exact API + UI smoke-test.

## Следующее действие

```powershell
pio run -e esp32 -t clean
pio run -e esp32
```

Build считать подтверждённым только после реального `SUCCESS`.

После `SUCCESS`:

```powershell
pio run -e esp32 -t upload
```

Затем полностью заменить microSD `/web` актуальным repo `firmware/esp32/web`. Не делать overlay поверх старого `/web`.

Минимальный runtime smoke test:

```text
/api/clients
/api/motors
/api/repairs
/api/clients/by-id?client_id=<known>
/api/motors/by-id?motor_id=<known>
/api/repairs/by-id?repair_id=<known>
```

Проверить основные mobile/desktop screens:

```text
clients
motors
repairs
costing
linked winding
reports
writeoff
materials
```

## Следующий performance review после успешного build/runtime

`appendRepairsPageJson()` уже ограничивает repair rows максимум 32, но текущий `repairClosed()` может сканировать status ledger отдельно для каждой строки страницы. Это потенциальный growing-I/O path.

Оптимизировать его только после успешного build/runtime текущего batch, предпочтительно через bounded/batched status resolution для одной страницы, сохранив exact-one CLOSED semantics и fail-closed поведение.

После этого снять populated-dataset timings через `/api/backup/manifest`.

Segmentation/rotation threshold вводить только по фактическим latency/size metrics. DB migration/destructive compaction без доказанной необходимости не вводить.

Основная новая функциональность сейчас не приоритет. Проект находится в production-hardening/performance phase.

## Closed checkpoint — batched repair-status paging (2026-08-12)

The previously identified repeated `repairClosed()` scan inside `appendRepairsPageJson()` is closed. The page reader now resolves up to 32 repair statuses through one authoritative status-ledger pass while preserving exact-one CLOSED and fail-closed semantics.

```text
6a64bf66045281bf2f37e8f9d7ad2250205f1369
52b6e2a69664bc150a51978b292b437079bd1933
ESP32 Build: SUCCESS
CMP Protocol Tests: SUCCESS
```

Next work must be selected from measured populated-dataset timings or a concrete correctness/fault-test result. Do not introduce a database migration, destructive compaction, or arbitrary rotation threshold without evidence.

## Completed: browser pagination of growing collections — 2026-08-12

Closed for mobile + desktop:

```text
clients              20/page, cursor Previous/Next
motors               20/page, cursor Previous/Next
repairs              20/page, cursor Previous/Next
winding history      20/page, cursor Previous/Next
monthly report rows  20 visible/page
repair selectors     exact selected client/motor only
costing winding scan streaming counters, no full event array
```

Verified code HEAD:

```text
df632cd17aec82af6861fcdcf552a3ef90e20224
ESP32 Build: SUCCESS
CMP Protocol Tests: SUCCESS
```

## Completed: materials and warehouse pagination — 2026-08-12

The remaining growing material and warehouse readers are now bounded at storage + HTTP boundaries:

```text
GET /api/materials               default 20, maximum 32
GET /api/materials/adjustments   default 20, maximum 32
GET /api/materials/usage         default 20, maximum 32
GET /api/warehouse/spools        default 20, maximum 32
GET /api/warehouse/write-offs    default 20, maximum 32
```

Mobile and desktop consumers now keep only the selected page and expose Previous/Next navigation for catalogues, histories and repair selectors. Write-off aggregate mass/value/material totals still scan and validate the complete repair history, while the response contains only the requested row page.

Verified code HEAD:

```text
53285422a646b488776013029a438f24faeabfc6
ESP32 Build: SUCCESS
CMP Protocol Tests: SUCCESS
```

Exact repair filters, spool/material semantics, strict provenance and fail-closed behavior remain unchanged. The next scaling action should be selected from populated-device timing/RAM measurements or a concrete fault-test result, not another speculative catalogue refactor.


## Completed: safe motor and winding import preparation — 2026-08-12

Implemented a backward-compatible sourced motor schema plus desktop/mobile JSON import pages. A batch is limited to 50 records in browser memory, preview is non-mutating, similarity checks run before import, and identity matches are skipped by default. Every sourced entry carries an explicit source category, title and confidence; calculated values remain visibly marked.

This prepares the database for researched data packages without treating web search results as verified factory truth. Actual collection/import should proceed in reviewed manufacturer/series batches.


## Completed: settings navigation and operability audit — 2026-08-12

All existing desktop/mobile maintenance pages are now reachable from Settings, and CI enforces embedded-JavaScript syntax plus internal-link integrity. Warehouse price and calculator settings are backed by validated persistent APIs; load failures are now shown as failures.

Wi-Fi/FTP audit result: the fixed local AP is implemented, but configurable STA profiles/static IP and the FTP server are still planned rather than operational. Their current pages intentionally remain read-only status/capability pages. Implementing those services is a separate firmware/security block requiring credential persistence, fallback/recovery and real-device network tests.


## Completed: verified ESP32 3 MiB application partition — 2026-08-13

The 4 MiB ESP32-D0WD-V3 target has no PSRAM. Arduino IDE and PlatformIO now use the matching `Huge APP (3MB No OTA/1MB SPIFFS)` / `huge_app.csv` layout. Local and CI builds both report 14.4% static RAM and 36.8% application Flash use.

Confirmed local build:

```text
RAM:   47336 / 327680 bytes (14.4%)
Flash: 1158929 / 3145728 bytes (36.8%)
ESP32 build: SUCCESS
```

Confirmed CI build at `fb656563da7737ebd51d2853b7f42c077790236d`:

```text
RAM:   47336 / 327680 bytes (14.4%)
Flash: 1158861 / 3145728 bytes (36.8%)
ESP32 Build: SUCCESS
Arduino Uno Build: SUCCESS
CMP Protocol Tests + web navigation audit: SUCCESS
```

There is now sufficient application-partition headroom for the bounded Wi-Fi manager and subsequently a restricted single-client FTP service. Runtime heap and real-device UART/SD concurrency still require measurement after each network stage.


## Completed: remote FTP backup configuration foundation — 2026-08-13

The local backup target is a TP-Link TL-WR942N hardware v1 with USB storage.
Router firmware remains replaceable: CoilMaster uses standard configurable FTP
rather than a vendor-specific interface.

Checkpoint:

```text
4f13a394b2b7467ec71b92022d1f36059c1d6919
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
```

Implemented:

- atomic versioned remote FTP configuration with recovery from `.tmp/.bak`;
- hidden credential semantics (`password_configured`, never password output);
- desktop/mobile settings UI;
- FTP greeting, USER/PASS and remote CWD test;
- STA prerequisite and `BackupActivityGuard` fail-closed blocking during active
  or unprovable machine state.

Exact next network work:

1. hardware-test the bounded DHCP AP+STA manager with TL-WR942N v1 while
   confirming the service AP remains at `192.168.4.1`;
2. implement actual outbound backup upload from the authoritative whitelist via
   temporary `.part`, size/integrity verification and final rename;
3. then implement the restricted single-client incoming FTP service, automatic
   only when `/web` is absent and otherwise operator-started from the web UI.

Do not describe remote backup upload or incoming FTP as operational until those
later runtime stages and real-device transfer tests are complete.

Wi-Fi checkpoint:

```text
d7a541acc2f752137783a0b6a0cfb6a86c4d727c
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
```

Maximum five DHCP profiles, priority ordering, hidden SSID, enable/disable,
masked passwords, atomic NDJSON recovery and bounded non-blocking attempts are
implemented. Static IP and nearby-network scan remain explicitly incomplete.

Remote FTP upload checkpoint:

```text
bc910d773a15f8c3fd5d7b261ae22046b2732826
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
```

Main manifest files can now be uploaded individually through an asynchronous
FTP state machine with deep snapshot audit, `.part`, exact `SIZE` check and
final rename. The next required step is real-device testing against TL-WR942N
v1, followed by a bounded complete-backup batch that includes paged session
files and versioned snapshot naming. Retention and scheduling must remain
disabled until batch completeness and interruption recovery are proven.
