# Где остановились и что делать дальше

Дата обновления: **2026-08-15**
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

Maximum five DHCP/static-IP profiles, priority ordering, hidden SSID,
enable/disable, masked passwords, atomic NDJSON recovery and bounded
non-blocking attempts are implemented. Nearby-network scan is also implemented
as an asynchronous bounded API returning at most 20 unique visible SSIDs.

Current network checkpoint:

```text
1710343ede275aa2c49c0f6ad84c6f55b663df53
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.5% (50712 / 327680 bytes)
Flash: 39.9% (1254225 / 3145728 bytes)
```

Static addressing and nearby scan still require real-router smoke tests. DHCP
remains the default and service AP `192.168.4.1` remains active.

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

Single-file hardware transfer to TL-WR942N v1 is now user-confirmed. The
complete batch described above has been implemented in:

```text
b2e1adac054ae88b1afc873eeac37fc9c53267a5
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
```

Exact next verification:

1. flash this firmware and update microSD `/web`;
2. press `Создать полную копию на сервере` while the machine is idle;
3. confirm files share one `cm-b<id>-` prefix;
4. confirm main files and session snapshot/spool-selection/state files exist;
5. confirm `cm-b<id>-COMPLETE.txt` is the last successful file;
6. interrupt one later test and confirm that the incomplete prefix has no
   `COMPLETE.txt` marker.

Do not enable retention until these checks pass.

## Completed in firmware: restricted incoming `/web` FTP — 2026-08-13

Implemented in `CM_WebRecoveryFtpServer` and integrated with `main.cpp`,
`CM_StaticSiteServer` and both FTP settings pages.

- automatic start is captured only when `/web` is absent at boot;
- otherwise only explicit operator start/stop is available;
- control address `192.168.4.1:21`, one client, no anonymous access;
- fixed recovery credentials `CoilMaster` / `CoilMaster123`;
- FTP `/` is strictly mapped to microSD `/web`; `/data` is unreachable;
- passive directory listing/upload, mkdir, delete and rename are supported;
- uploads use `.part`, target `.bak` rollback and final rename;
- start and runtime remain fail-closed on the shared winding activity probe;
- fallback HTTP page displays recovery instructions when `index.html` is absent.

Verified checkpoint:

```text
2505e1f06a55cf251cfb54614e99d4c6277c5c8e
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 14.8% (48352 / 327680 bytes)
Flash: 38.6% (1215773 / 3145728 bytes)
```

Exact next incoming-FTP verification on hardware:

1. use a test microSD without `/web` and confirm automatic server start;
2. connect to Wi-Fi `CoilMaster`, then FTP `192.168.4.1:21` with the recovery account;
3. upload the complete contents of `firmware/esp32/web`, including subfolders;
4. confirm an interrupted upload leaves no replaced/truncated target;
5. reboot and confirm the site loads while FTP remains stopped;
6. start and stop FTP manually from desktop/mobile settings while safely idle;
7. confirm invalid login, traversal and a non-AP client cannot access files;
8. start a controlled machine activity transition and confirm FTP stops.

Do not call the incoming FTP hardware-confirmed until this matrix passes. The
complete outgoing backup batch interruption test and retention gate remain
separate pending work.

## Completed in firmware: local mDNS address — 2026-08-15

`main.cpp` starts `ESPmDNS` after the bounded AP+STA manager and advertises
`_http._tcp` on port 80. `CM_StaticSiteServer` exposes the runtime result in
`/api/system/network`. The intended URL is `http://coil.local/`; direct
`http://192.168.4.1/` access remains available at all times when the service AP
is active.

Verified checkpoint:

```text
b2205640478c6b24509389ef17794e76036709d6
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.3% (50288 / 327680 bytes)
Flash: 39.5% (1243269 / 3145728 bytes)
```

Hardware verification remains required:

1. open `http://coil.local/` while connected directly to Wi-Fi `CoilMaster`;
2. connect ESP32 STA and the test client to the same router LAN and repeat;
3. confirm `/api/system/network` returns `mdns_active:true`;
4. confirm `http://192.168.4.1/` still works if `.local` resolution is blocked;
5. record router guest/client-isolation behavior separately, because those
   modes may intentionally block multicast UDP 5353.

## Closed: missing desktop sidebar icons — 2026-08-15

The missing-icon issue was caused by inconsistent historical sidebar markup and
compact CSS that hides anchor text. It is fixed centrally in the HTML script
appended by `CM_StaticSiteServer`; no per-page navigation fork was introduced.
Existing icons are preserved, missing icons are assigned by link/label, and the
icon has its own responsive font size. The injected script is now syntax-checked
by the web asset audit.

```text
c065ede2ebf439cc55a95bff4cd460e4709cfef2
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation/injected-script audit: SUCCESS
```

## Hardware confirmed: backup and FTP — 2026-08-15

The user confirmed on the real device that both the outgoing remote backup and
the restricted incoming `/web` FTP service execute successfully. These two
features are no longer implementation-only checkpoints. The earlier detailed
negative/security matrix remains useful for regression testing, but does not
block calling the basic backup/FTP paths hardware-operational.

## Completed in firmware: safe remote backup retention — 2026-08-15

Retention is now applied only after a new full batch has uploaded its
`cm-b<id>-COMPLETE.txt` marker successfully. Each new batch records an exact
local manifest under `/data/settings/remote-backup-batches`; retention never
lists or guesses arbitrary files on the FTP server.

Deletion order is fail-safe:

1. delete the selected old batch `COMPLETE.txt` marker first;
2. delete only exact `cm-b<id>-...` names from its trusted local manifest;
3. remove the local manifest only after all exact remote deletes finish;
4. repeat until `retention_count` is satisfied.

FTP `550` for an already absent exact managed file is treated as idempotent
success, allowing an interrupted cleanup to resume after the next successful
backup. A retention failure does not invalidate the newly completed backup and
is shown separately in the web UI. Legacy remote batches created before this
feature are intentionally not deleted automatically because no trusted local
manifest exists for them. Incomplete `.tmp` manifests are ignored.

Verified code checkpoint:

```text
b7860d6327dedba14cda826c1bf0fc91fb99f040
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.5% (50760 / 327680 bytes)
Flash: 40.1% (1261073 / 3145728 bytes)
```

Hardware retention is now user-confirmed on the real device. Creating more
complete backups than the configured limit correctly removes the oldest
managed batch. The FTP-session reuse fix that closed the multi-delete failure
is recorded in:

```text
4f8261da92f581fdbfb45351f5987fa31a75eed7
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.5% (50760 / 327680 bytes)
Flash: 40.1% (1261397 / 3145728 bytes)
```

## Completed: manual application of backup retention — 2026-08-15

The backup page now exposes `Применить лимит копий сейчас`. It applies the
saved `retention_count` without creating an extra full backup. The endpoint is
fail-closed on machine activity, requires configured/enabled remote backup and
an active STA connection, and reuses the exact trusted-manifest deletion path.
Legacy or arbitrary FTP files remain outside automatic deletion.

```text
dd5e52bfa984471844c1bb38c5db21098e08a129
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.5% (50760 / 327680 bytes)
Flash: 40.1% (1262133 / 3145728 bytes)
```

Runtime smoke test after flashing: change the saved limit to a value below the
current managed-batch count, press the new button while the machine is idle,
and confirm the UI reports completion without creating a new `cm-b<id>-`
prefix.

## Completed in firmware: interrupted remote-batch cleanup — 2026-08-15

Every intended remote batch filename is now appended and flushed to the local
temporary manifest before its FTP upload starts. A failed batch keeps that
`.tmp` manifest instead of discarding the only exact cleanup evidence.

The next successful full backup or the manual retention action first processes
old incomplete manifests. For each canonical batch it deletes the exact
`COMPLETE.txt` name first, then only the manifest-recorded final names and their
`.part` variants. FTP directory listing, prefix guessing and deletion of legacy
or unrelated router files remain prohibited. The local `.tmp` manifest is
removed only after all exact idempotent deletes finish.

```text
1984011d80ee92a904b923bf8c56b219d7251556
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.5% (50784 / 327680 bytes)
Flash: 40.2% (1264201 / 3145728 bytes)
```

The hardware fault test is user-confirmed: after interruption and reboot the
manual retention action removed the interrupted batch final/`.part` files,
created no new batch prefix and preserved completed backups.

## Completed in firmware: DS3231 runtime diagnostics — 2026-08-15

ESP32 now initializes the documented DS3231 on GPIO21/GPIO22 without an
external `RTClib` dependency. The bounded reader validates BCD fields, calendar
ranges, 12/24-hour mode and the DS3231 oscillator-stop flag. It never reports a
lost-power clock value as valid.

```text
GET /api/system/time
source: DS3231
detected
time_valid
local_time: YYYY-MM-DDTHH:MM:SS or null
timezone_configured: false
scheduling_ready: false
```

Desktop and mobile Settings load the same shared status component and show the
detected module and current validated local time. No RTC write or automatic
backup schedule is enabled in this checkpoint.

```text
1ef7d428b689bdda21a75fc390886bc093c337e4
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.6% (51128 / 327680 bytes)
Flash: 40.8% (1284073 / 3145728 bytes)
```

The DS3231 runtime read path is hardware-confirmed by the user: Settings detects
the module and displays the current time correctly.

## Completed in firmware: verified operator RTC synchronization — 2026-08-15

Desktop and mobile Settings now provide `Установить время с этого устройства`.
The action requires an explicit browser confirmation and sends local browser
date/time to `POST /api/system/time`. The server rejects the write unless the
machine is provably idle, validates every calendar field, writes DS3231 in
24-hour mode, clears the oscillator-stop flag and reads the clock back. The
write succeeds only when the verified RTC value is within two seconds of the
requested value.

No automatic time write, NTP sync or backup schedule is introduced here.

```text
88efc72342bd5a7e604fee21fa207d34e1dc4e7c
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.6% (51128 / 327680 bytes)
Flash: 40.9% (1285869 / 3145728 bytes)
```

Hardware check: while the machine is idle, deliberately change the RTC time,
press the synchronization button and confirm Settings immediately shows the
phone/computer local time. Also confirm that the same POST is rejected while
the activity state is busy or unavailable.

## Completed in firmware: safe daily remote-backup schedule — 2026-08-15

Remote backup settings schema 2 adds an optional daily local DS3231 time. Schema
1 files remain readable and migrate atomically on the next settings save.

The scheduler checks at a bounded 30-second interval and starts the existing
full manifest backup only when all gates are true:

- remote backup and schedule are enabled;
- DS3231 date/time is valid;
- the configured local time has arrived;
- STA is connected to the router;
- machine state is provably idle;
- the same RTC date has not already completed a scheduled batch.

If Wi-Fi or the safe-idle condition is unavailable, the scheduler waits. Once
the expensive stable-snapshot attempt begins, it is not repeatedly retried on
the same boot/date after a failure. A completed batch stores
`last_scheduled_date` atomically; rebooting later that day cannot create a
duplicate scheduled copy. A missed time after reboot is caught up once the
gates become safe.

The scheduled path reuses the exact full-batch, COMPLETE marker, interrupted
batch cleanup and retention state machines. It has no connection to physical
START, Arduino SSR control, auto-resume or wire writeoff.

```text
0b6c0613957f5faf7c2f3a1ed41af447f8eabe99
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.6% (51168 / 327680 bytes)
Flash: 41.0% (1289409 / 3145728 bytes)
```

Hardware test: set the daily time two or three minutes ahead, enable the
schedule and leave the machine safely idle. Confirm exactly one new complete
batch appears, Settings reports `Плановая копия за сегодня выполнена`, and a
reboot on the same RTC date does not create a second scheduled batch. Then set
the intended production time.

The scheduled-backup hardware checkpoint is user-confirmed on the real device.

## Completed in firmware: self-contained remote backup manifest — 2026-08-15

Every new full remote backup now uploads `cm-b<id>-MANIFEST.txt` after all data
files and before `cm-b<id>-COMPLETE.txt`. The manifest is generated by streaming
the trusted local temporary name ledger and contains:

```text
COILMASTER_BACKUP_MANIFEST_V1
batch_id=<id>
data_files=<exact count>
<exact remote data filename, one per line>
```

Names are revalidated against the exact batch prefix and reject path
separators, traversal, control characters, the manifest name itself and the
completion marker. The recorded count must equal the number of successfully
uploaded data files. The uploaded manifest is itself added to the local cleanup
ledger before transfer, so interruption recovery and retention remove it by an
exact trusted name. `COMPLETE.txt` remains the final validity marker.

```text
7851c5360f439a3d1fca20101eef090098b24b97
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.6% (51168 / 327680 bytes)
Flash: 41.1% (1291389 / 3145728 bytes)
```

Hardware test: create one full backup and verify that the matching
`MANIFEST.txt` exists before `COMPLETE.txt`, its `batch_id` matches the prefix,
its `data_files` count equals the following filename lines, and every listed
file exists on the FTP server. An interrupted batch without `COMPLETE.txt`
must remain invalid and its manifest/data/`.part` files must be removed by the
next full backup or manual retention action.

This hardware checkpoint is user-confirmed on the real device: the manifest,
exact file list/count, final COMPLETE marker and interrupted-batch cleanup all
behave as specified.

## Completed in firmware: read-only remote backup metadata inspection — 2026-08-15

Desktop and mobile backup pages now accept an exact numeric batch ID and expose
`Проверить копию без восстановления`. The operation downloads only the selected
`cm-b<id>-MANIFEST.txt` and `cm-b<id>-COMPLETE.txt` into a fixed temporary
settings directory. It does not download data files, apply a restore, overwrite
business data or change winding state.

The bounded FTP RETR path requires binary mode and a canonical filename, reads
the server's exact SIZE first, limits the manifest to 128 KiB and the marker to
512 bytes, writes through a local `.part`, checks the received byte count and
renames atomically. It remains guarded by the safe-idle runtime probe throughout
the transfer.

Validation requires manifest version 1, the requested exact batch ID, a bounded
declared data-file count, canonical same-prefix filenames, and a matching final
COMPLETE marker whose `files_before_marker` equals data files plus the manifest.
This is metadata inspection only; remote presence/content verification of every
listed data file is a later stage before restore can be enabled.

```text
0e054aab1586ef843a70e3830585d351c6c791b5
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.6% (51232 / 327680 bytes)
Flash: 41.3% (1299113 / 3145728 bytes)
```

Hardware test: update microSD `/web`, flash the firmware, open Backup, enter the
ID from an existing `cm-b<id>-MANIFEST.txt`, and press the inspection button.
Confirm the UI reports a valid manifest/COMPLETE pair and explicitly says that
restore was not performed. Then try a nonexistent ID and an interrupted batch;
both must fail while all working data remains unchanged.

This metadata-inspection hardware checkpoint is user-confirmed on the real
device.

## Completed in firmware: remote file presence and size verification — 2026-08-15

New full backups use `COILMASTER_BACKUP_MANIFEST_V2`. Every data-file line now
contains its canonical remote name and exact expected byte size separated by a
tab. The local cleanup/retention manifests use the same entry format, while all
readers remain backward-compatible with legacy name-only manifests.

After validating MANIFEST and COMPLETE, the inspection state machine probes
every listed remote file with FTP `SIZE` through one reusable authenticated
control session. V2 requires every returned size to equal the recorded size.
V1 remains accepted and verifies that every listed name exists, but the UI
explicitly reports that historical expected sizes are unavailable. No data file
is downloaded and no restore is performed.

```text
584030f4b60b89b80eec081fb0870dd2850d1ca0
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.6% (51272 / 327680 bytes)
Flash: 41.4% (1301997 / 3145728 bytes)
```

Hardware test: create a new full backup after flashing and updating `/web`,
then inspect its ID. The UI must reach `FILES`, count from zero to the exact
manifest total, and finish with `наличие и размеры всех файлов подтверждены`.
Inspect one older V1 batch and confirm it succeeds with the explicit
presence-only warning. For a controlled negative test, rename one data file in
a disposable copied batch or alter its expected V2 size; inspection must fail
without changing any working data.
