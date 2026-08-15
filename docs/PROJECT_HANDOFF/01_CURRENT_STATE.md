# Текущее состояние CoilMaster

Дата обновления: **2026-08-12**  
Ветка: **`cmp-protocol-v1`**  
Статус: **основной production workflow собран; Arduino local path реально проверен; ESP32 clean build подтверждён; продолжается backup/E2E доводка**

## Source of truth

Единственный источник кода — `cmp-protocol-v1`. `main` не использовать как источник реализации.

Перед изменением существующего файла всегда заново fetch текущую версию из `cmp-protocol-v1` и использовать актуальный blob SHA.

## Safety boundary

CoilMaster объединяет Arduino Uno и ESP32:

- Arduino Uno — real-time winding, SSR, Hall, физический START, keypad/LCD/buzzer;
- ESP32 — WEB, microSD, задания, workshop registry, warehouse/materials, costing, reports, autonomous archive и backup;
- UART — job delivery и winding events.

Safety invariants:

- ESP32/Web не управляют SSR напрямую;
- automatic physical START отсутствует;
- после reboot нет auto-resume;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся ручным и связан с exact `spool_id + source_session_id + source_run_id`.

## Основной production flow

```text
client
→ motor + authoritative coil_program
→ OPEN repair
→ costing
→ linked winding
→ exact ACTIVE spool_id
→ immutable job snapshot + immutable spool selection
→ UART delivery
→ JOB_ACK ACCEPTED
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ manual exact-run wire writeoff
→ materials / pricing
→ read-only finalization preflight
→ CLOSED
→ reports/archive
→ read-only backup
```

## Подтверждённый Arduino hardware checkpoint

После SRAM-оптимизации Arduino Uno больше не находится в reset-loop.

Подтверждённый runtime:

```text
CM_BOOT stage=READY free_sram=357
CM_ALIVE ... free_sram=366
```

Пользователь реально проверил:

```text
создание программы на Arduino
→ подтверждение keypad
→ physical START
→ выполнение намотки
→ LOCAL_EVT
→ доставка на ESP32
```

Постоянный buzzer tone был аппаратной ошибкой подключения: buzzer был на D12 (SSR) вместо D11. После переноса проблема устранена.

Pin-map:

```text
D11 = Buzzer
D12 = SSR
A0  = Hall
A1  = Arduino TX → ESP32 RX
A2  = Arduino RX ← ESP32 TX
```

## Подтверждённый ESP32 build checkpoint

Clean local PlatformIO build текущего ESP32 firmware подтверждён пользователем 2026-08-12:

```text
RAM:   14.4% (used 47320 bytes from 327680 bytes)
Flash: 86.7% (used 1136229 bytes from 1310720 bytes)
SUCCESS Took 31.04 seconds
```

Это подтверждает compile/link текущего ESP32 кода, включая UART hardening, autonomous archive integrity, backup integration и server-side UI switch footer. Это не заменяет hardware/runtime E2E и не является GitHub CI result.

## UART protocol

Обычный web-linked event:

```text
CMP1|EVT|RUN_STARTED|session|run|0|CRC
CMP1|EVT|RUN_COMPLETED|session|run|completed|CRC
```

Standalone Arduino event:

```text
CMP1|LOCAL_EVT|RUN_STARTED|session|run|0|WORKING|coil_count|turns|CRC
CMP1|LOCAL_EVT|RUN_COMPLETED|session|run|completed|WORKING|coil_count|turns|CRC
```

`WORKING` может быть `STARTING`.

ESP32 parser fail-closed требует:

```text
RUN_STARTED   -> completed_runs == 0
RUN_COMPLETED -> completed_runs > 0
```

Никакой event сам по себе не запускает physical START и не списывает провод.

## Автономные намотки Arduino

Отдельный workflow для задач, созданных и выполненных непосредственно на Arduino:

```text
Arduino local keypad
→ physical START
→ LOCAL_EVT
→ ESP32 autonomous archive
→ completed/incomplete status
→ поиск программы ±20%
→ existing/similar motor
→ ручная assignment completed task → motor
```

Persistent files:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

Event сохраняет:

- exact `session_id/run_id`;
- `RUN_STARTED` или `RUN_COMPLETED`;
- `WORKING/STARTING`;
- coil count;
- canonical program;
- completed runs;
- `start_observed`;
- receive uptime.

Incomplete task можно искать и просматривать, но нельзя назначить как completed evidence.

Если ESP32 пропустила START, но Arduino позднее replay-ит persisted completion, запись сохраняется с `start_observed=0` и UI показывает предупреждение.

Assignment log append-only. Completed autonomous tasks можно группировать на одном motor с ролями `WORKING`, `STARTING`, `AUXILIARY`. Исходные Arduino events не переписываются.

## Намотка и persisted linked state

Реализованы:

- persistent `job_id/session_id` allocator;
- immutable job snapshot;
- immutable exact spool selection;
- persisted runtime state;
- recovery/manual-review flow;
- strict repair/motor linkage;
- authoritative `coil_program` parser;
- winding journal schema 2;
- semantic `RUN_STARTED → RUN_COMPLETED` audit;
- exact-run provenance для ручного wire writeoff.

Session persistence:

```text
/data/winding-jobs/id-state.txt
/data/winding-jobs/id-state.bak
/data/winding-jobs/snapshots/session-<id>.json
/data/winding-jobs/spool-selection/session-<id>.json
/data/winding-jobs/state/session-<id>.json
/data/winding-runs/events.ndjson
```

Deep winding validation использует authoritative `WindingJournalQuery::validateAll()` до EOF и затем `WindingJournalTransitionAudit::validate()`.

## Exact spool и wire writeoff

Новый linked job требует конкретную ACTIVE CU/AL бухту с положительным остатком. Выбор сохраняется immutable до UART delivery.

После `RUN_COMPLETED` оператор вручную вводит фактический расход/остаток. Server-side доказывает:

- exact run завершён;
- repair/spool совпадают с immutable selection;
- второй CONFIRMED для того же `(session, run)` запрещён;
- recovery сохраняет provenance через `PENDING → CONFIRMED | ABORTED`.

Exact-session `spool-selection/session-<id>.json.tmp` блокирует writeoff fail-closed.

## CLOSED / finalization

Перед `OPEN → CLOSED` backend проверяет:

- нет unfinished/recovery winding job;
- costing/material/warehouse persistence цела;
- winding journal и transitions целы;
- каждый новый completed linked run покрыт ручным exact-run writeoff.

Preflight read-only. Реальный close повторяет проверки независимо.

## Warehouse / materials / costing

Реализованы:

- CU/AL + legacy UNKNOWN;
- spool catalogue;
- recoverable spool swap;
- warehouse writeoff `PENDING → CONFIRMED | ABORTED`;
- material catalogue/usage/adjustments + recovery;
- warehouse price history;
- strict persisted parsing/reference lookup;
- RepairCosting с currency/formula/overflow/transaction checks.
- pricing history API/UI с обязательной пагинацией: default 20, max 32,
  opaque byte-offset cursor; полный журнал продолжает проверяться для
  `total_count/latest/current pricing` consistency.

## Mobile / desktop UI

Основные разделы доступны в обеих версиях.

Дополнительно переключатель mobile ↔ desktop теперь централизованно добавляется `CM_StaticSiteServer` в конец каждой `/mobile/...` и `/desktop/...` HTML-страницы. Он сохраняет текущий path/query/hash, поэтому пользователь может вернуться на ту же страницу в другой версии интерфейса.

Этот блок подтверждён пользователем в реальной работе.

## Network and local backup target

The service AP has been extended to a bounded non-blocking AP+STA manager. Up to
five DHCP profiles are stored atomically and attempted by priority while the
`CoilMaster` AP remains at `192.168.4.1`. Passwords are never returned through
the API. Desktop/mobile pages manage the bounded profiles.

The external backup target is a TP-Link TL-WR942N hardware v1. Remote FTP
configuration and greeting/login/CWD testing are implemented. Actual backup
upload and incoming recovery FTP are not implemented yet.

```text
d7a541acc2f752137783a0b6a0cfb6a86c4d727c
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
```

Remote backup has since gained asynchronous per-file upload for the fixed main
manifest whitelist. It uses `.part`, exact FTP `SIZE` verification and final
rename, and aborts if runtime winding activity becomes busy/unprovable. Session
files and complete versioned batches remain incomplete.

```text
bc910d773a15f8c3fd5d7b261ae22046b2732826
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
```

The single-file FTP transfer has now been confirmed by the user on the real
TL-WR942N v1. A complete asynchronous batch is also implemented: main whitelist
plus all session snapshot/spool-selection/state files, a persistent `cm-b<id>`
prefix and a final `COMPLETE.txt` marker. The complete batch is compile/CI
confirmed but still requires hardware execution.

```text
b2e1adac054ae88b1afc873eeac37fc9c53267a5
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
REAL COMPLETE-BATCH TEST: PENDING
```

Incoming website recovery FTP is now implemented. At boot the absence of
`/web` is captured before the service creates that directory, and the server
starts automatically on the fixed AP at `192.168.4.1:21`. If `/web` already
exists it stays stopped until the operator uses the desktop/mobile FTP settings
page. The fixed recovery account is `CoilMaster` / `CoilMaster123`.

The service accepts one client from the CoilMaster AP subnet, maps FTP `/`
strictly to microSD `/web`, rejects traversal, exposes no `/data` path, and has
no anonymous login. It supports directory upload through passive FTP. `STOR`
writes to `.part` and only replaces the target by rename after a complete data
connection; an existing target is temporarily preserved as `.bak`. Runtime
activity is fail-closed: the server cannot start unless safe idle is proven and
is stopped if that condition changes. It never controls Arduino, SSR or START.

```text
2505e1f06a55cf251cfb54614e99d4c6277c5c8e
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 14.8% (48352 / 327680 bytes)
Flash: 38.6% (1215773 / 3145728 bytes)
REAL INCOMING-FTP DIRECTORY/INTERRUPTION TEST: PENDING
```

## Read-only backup/export

Endpoints:

```text
GET /api/backup/manifest
GET /api/backup/file?name=...
GET /api/backup/sessions
GET /api/backup/session-file?kind=...&session_id=...
```

Arbitrary filesystem paths запрещены. Export/deep audit блокируется во время active winding.

Static whitelist включает workshop, winding, warehouse, materials, pricing, allocator, conductor settings и теперь также:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

Для autonomous archive добавлен read-only authoritative audit:

```text
AutonomousWindingArchive::validateStorage(...)
```

Он не вызывает `begin()`, не создаёт каталоги и не меняет microSD.

При failure:

```text
snapshot_stability_reason = autonomous_winding_archive_unstable_or_invalid
```

Manifest дополнительно содержит:

```text
autonomous_winding_archive_audit_duration_ms
autonomous_winding_event_record_count
autonomous_winding_started_record_count
autonomous_winding_completed_record_count
autonomous_winding_assignment_record_count
```

Per-domain timing теперь включает 10 полей:

```text
persistent_id_audit_duration_ms
conductor_settings_audit_duration_ms
material_persistence_audit_duration_ms
business_data_audit_duration_ms
autonomous_winding_archive_audit_duration_ms
winding_persistence_audit_duration_ms
warehouse_persistence_audit_duration_ms
warehouse_movements_audit_duration_ms
winding_session_directory_scan_duration_ms
winding_session_persistence_audit_duration_ms
```

Stage 0 observability теперь содержит **34 metrics**: прежние 29 + autonomous audit duration + 4 autonomous record counters.

При safe machine-state `snapshot_stable=true` означает успешную read-only проверку всего backup whitelist и необходимых recovery/session adjuncts, включая autonomous Arduino archive.

## Performance strategy

Не вводить database migration, arbitrary rotation threshold или persistent optimistic cache без измерений.

На реальном dataset сначала сохранить manifest с:

- `items[].size_bytes`;
- `snapshot_stability_duration_ms`;
- все per-domain durations;
- record counts;
- session file counts/bytes;
- allocator high-water.

Оптимизировать только подтверждённый hotspot.

## Что ещё не подтверждено

Текущий ESP32 clean local build **подтверждён**. GitHub CI остаётся неподтверждённым:

```text
LOCAL ESP32 BUILD: SUCCESS
CI NOT CONFIRMED
```

Arduino local hardware path подтверждён, но полный linked production E2E ещё нужен:

```text
linked repair
→ exact spool
→ JOB_ACK
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ manual exact-run wire writeoff
→ costing
→ finalization preflight
→ CLOSED
→ stable backup
```

Отдельно проверить negative/fault cases: reboot/manual review, microSD loss, corrupted persistence, UART timeout/reject/duplicate, wrong spool/session/run, duplicate writeoff, close without coverage и backup during active winding.

## Workshop repair-status paging checkpoint — 2026-08-12

Paged repair responses no longer scan `/data/workshop/repair-status.ndjson` once per returned repair. A bounded page (maximum 32 repairs) is collected first, then all current CLOSED states are resolved in one strict forward pass.

Commits:

```text
6a64bf66045281bf2f37e8f9d7ad2250205f1369  Batch repair status resolution for paged registry
52b6e2a69664bc150a51978b292b437079bd1933  Resolve paged repair statuses in one scan
```

Preserved semantics:

- full flat-JSON validation of the status ledger;
- required newline termination;
- exact-one CLOSED occurrence for every repair in the page;
- fail-closed response on storage/integrity failure;
- unchanged bounded API and NDJSON storage format.

Verification for code HEAD `52b6e2a69664bc150a51978b292b437079bd1933`:

```text
ESP32 Build: SUCCESS
CMP Protocol Tests: SUCCESS
```

## Browser-visible pagination checkpoint — 2026-08-12

The main growing UI collections no longer reconstruct an entire paged registry/archive in browser memory.

Paged at 20 visible records with cursor-backed Previous/Next navigation in both mobile and desktop:

- clients;
- motors;
- repairs;
- winding history;
- monthly report rows.

Repair creation now loads only the selected client and motor through exact `/by-id` endpoints instead of downloading both complete catalogs into `select` elements. Costing scans the exact repair winding archive page-by-page using streaming counters and latest-completion state rather than storing every event.

A pre-existing invalid desktop repair winding-program regex was also corrected.

Code checkpoint:

```text
df632cd17aec82af6861fcdcf552a3ef90e20224
ESP32 Build: SUCCESS
CMP Protocol Tests: SUCCESS
```

The monthly financial total intentionally still verifies every CLOSED repair in the selected month; only visible report rows are paged. This preserves authoritative totals while bounding rendered DOM size.


## Materials and warehouse pagination checkpoint — 2026-08-12

The remaining unbounded catalogue/history endpoints now use bounded cursor pages at the storage and HTTP boundaries:

- `GET /api/materials`;
- `GET /api/materials/adjustments`;
- `GET /api/materials/usage`;
- `GET /api/warehouse/spools`;
- `GET /api/warehouse/write-offs`.

Default page size is 20 records and the hard server maximum is 32. Desktop and mobile provide independent Previous/Next controls for material catalogues, adjustment history, repair material selection, active spools, spool selection and repair write-off history.

Warehouse write-off mass, value and material-group totals still cover every confirmed movement for the repair even though only one page of rows is returned. Exact `spool_id + source_session_id + source_run_id` write-off provenance and all existing fail-closed safety rules are unchanged.

Code checkpoint:

```text
53285422a646b488776013029a438f24faeabfc6
ESP32 Build: SUCCESS
CMP Protocol Tests: SUCCESS
```


## Safe motor database import checkpoint — 2026-08-12

CoilMaster now has a review-first JSON import path for motor and winding records. The persistent motor record supports nameplate data, winding geometry, conductor data, stator dimensions, source citation, confidence classification, and an explicit calculated-fields flag.

The import preview is read-only, checks the complete package locally, and queries the bounded similarity endpoint before any write. Exact identity matches are unchecked by default. Import remains an explicit user-confirmed sequence of individual append operations; there is no autonomous data ingestion.

Format and example:

- `docs/MOTOR_IMPORT_FORMAT.md`
- `docs/examples/motor-import.example.json`


## Settings and navigation audit — 2026-08-12

Desktop and mobile settings now link to motor import, material catalogue, backup export, pricing audit, winding history, autonomous Arduino archive, recovery, Wi-Fi and FTP status pages.

The warehouse price and conductor conversion settings were traced through their real GET/POST handlers and persistent stores. Their UIs now reject non-success GET responses instead of presenting an API error as an unconfigured/default value.

The older capability audit below is superseded by the network checkpoints in
this file. Current network capability boundary is:

- fixed password-protected `CoilMaster` fallback AP is implemented and starts in firmware;
- up to five saved DHCP/static-IP STA profiles and priority selection are implemented;
- asynchronous nearby-network scanning is implemented with 20 unique visible SSIDs per result;
- restricted incoming `/web` FTP and its start/stop/status controls are implemented;
- incoming FTP credentials are fixed recovery credentials and are intentionally
  shown in the trusted-local-network UI; no business-data FTP root exists.

CI now runs `Tests/Web/check_web_assets.js` to compile every embedded script, reject duplicate HTML IDs, and reject missing static internal page targets.

Verified code checkpoint:

```text
a2faccb7a19bb4e96cf6982c93af241cc46da6f6
ESP32 Build: SUCCESS
CMP Protocol Tests + web navigation audit: SUCCESS
```


## ESP32 application partition baseline — 2026-08-13

The physical target was measured on-device:

```text
ESP32-D0WD-V3
physical flash: 4 MiB
PSRAM: absent
CPU: 2 cores, 240 MHz
flash mode: QIO, 80 MHz
```

Arduino IDE `Huge APP (3MB No OTA/1MB SPIFFS)` was tested successfully on the device. PlatformIO now pins the equivalent layout:

```ini
board_build.partitions = huge_app.csv
```

Verified at code commit `fb656563da7737ebd51d2853b7f42c077790236d`:

```text
ESP32 Build: SUCCESS
RAM:   14.4% (47336 / 327680 bytes)
Flash: 36.8% (1158861 / 3145728 bytes)
Arduino Uno Build: SUCCESS
CMP Protocol Tests + web navigation audit: SUCCESS
```

OTA is unavailable with this layout. CoilMaster is updated through USB; website and business data remain on microSD. This change does not alter START, reboot recovery, SSR ownership, or manual write-off rules.

## Local `coil.local` hostname — 2026-08-15

The ESP32 now starts an mDNS responder with host `coil` and advertises the HTTP
service on TCP port 80. While the client is connected to the CoilMaster AP or
to the same LAN as the ESP32 STA interface, the normal site can be opened at:

```text
http://coil.local/
```

`GET /api/system/network` reports `mdns_supported`, `mdns_active`,
`local_hostname` and `local_url`. The fixed AP address `http://192.168.4.1/`
remains the mandatory fallback; mDNS is not a replacement for IP access. Some
routers, guest networks, client-isolation modes or clients that block multicast
UDP 5353 may not resolve `.local` names.

```text
b2205640478c6b24509389ef17794e76036709d6
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
RAM: 15.3% (50288 / 327680 bytes)
Flash: 39.5% (1243269 / 3145728 bytes)
REAL AP/STA mDNS TEST: PENDING
```

## Desktop navigation icon normalization — 2026-08-15

Desktop pages previously used inconsistent sidebar markup: some links already
contained emoji icons while many secondary pages contained text only. Compact
sidebars also set the anchor font size to zero, which made text-only entries
appear empty. `CM_StaticSiteServer` now normalizes every `aside a` on streamed
desktop HTML, preserves explicit existing icons, assigns missing icons from the
route/label and renders icon and label in separate spans. The icon therefore
keeps a visible size when the compact layout hides the label.

`Tests/Web/check_web_assets.js` now compiles the injected PROGMEM UI script and
requires the desktop icon normalizer in addition to checking all 48 HTML files.

```text
c065ede2ebf439cc55a95bff4cd460e4709cfef2
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation/injected-script audit: SUCCESS
```


## Current v1 completion estimate and motor-import verification — 2026-08-15

The completion estimate is **90%** against the defined CoilMaster v1 target: a
deployable workshop controller with the established physical-START safety
boundary, linked repair/winding flow, warehouse accounting, bounded UI, network
services and recoverable backups. It is not an estimate against every possible
future catalogue, analytics or automation feature.

| Area | Weight | Complete |
|---|---:|---:|
| Winding hardware, UART and safety invariants | 20% | 95% |
| Client → repair → winding → manual writeoff → close flow | 20% | 94% |
| Persistent data integrity, exact lookup and pagination | 15% | 93% |
| Warehouse, calculator and reviewed motor import | 15% | 90% |
| Wi-Fi, FTP, scheduled backup and recovery preparation | 15% | 88% |
| Final hardware acceptance, recovery drill and release documentation | 15% | 82% |

Weighted result: **91%**.

The old motor/winding JSON importer was re-audited and hardened at
`684e848c235b5f37607e9ca814e8bc11647c1b5d`. Desktop and mobile now reject
unknown fields, impossible source dates, non-HTTP(S) source URLs, oversized text,
calculated-provenance mismatches and duplicate identities inside one package.
Successfully created rows are disabled in the current preview so they cannot be
submitted twice accidentally. Firmware repeats the text, date, URL, required
source metadata and calculated-provenance checks before appending a motor.

The web audit now executes the documented example and negative validation cases
for both UIs. CMP Protocol Tests and ESP32 Build are confirmed successful:

```text
RAM:   15.7% (51408 / 327680 bytes)
Flash: 41.8% (1314657 / 3145728 bytes)
```

This is code/CI confirmation. A real-device import of a small disposable reviewed
package is still required before calling the importer hardware-confirmed.


## Completed in firmware: verified local rollback snapshot — 2026-08-15

After a V2 batch has been inspected, staged and mapped by the strict restore plan,
the operator can explicitly create a local safety snapshot of the current working
files. The snapshot is stored only under
`/data/settings/remote-restore-rollback`.

For every fixed target in the plan, the state machine records either the current
file or an explicit `MISSING` state. Present files are copied through a local
`.part`, read back and checked with CRC32 before they are added to the atomic
rollback manifest. Work is bounded to 1 KiB per update step. Machine activity
becoming busy or unavailable fails closed and removes the incomplete rollback
directory.

Completion creates `FILES.tsv` and `ROLLBACK.txt` with
`restore_apply_enabled=0` and `restore_applied=0`. No working path is opened
for writing, no staged file is applied, and reboot never resumes or applies the
snapshot. A surviving directory is reported as `STALE` and requires explicit
cleanup.

```text
1e7af6c3031f513b97cbfa974c922cd593d02c78
CMP Protocol Tests + web asset audit: SUCCESS
ESP32 Build: SUCCESS
RAM: 15.7% (51608 / 327680 bytes)
Flash: 42.1% (1324989 / 3145728 bytes)
```

This checkpoint is hardware-confirmed by the user on the real device. Creation,
CRC32 completion, unchanged working data, reboot `STALE` behavior and explicit
cleanup all passed. The rounded CoilMaster v1 estimate is now **91%**.
