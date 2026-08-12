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

## Mobile / desktop UI

Основные разделы доступны в обеих версиях.

Дополнительно переключатель mobile ↔ desktop теперь централизованно добавляется `CM_StaticSiteServer` в конец каждой `/mobile/...` и `/desktop/...` HTML-страницы. Он сохраняет текущий path/query/hash, поэтому пользователь может вернуться на ту же страницу в другой версии интерфейса.

Этот блок подтверждён пользователем в реальной работе.

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
