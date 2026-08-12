# Chat continuation checkpoint — 2026-08-12

Ветка: **`cmp-protocol-v1`**  
Репозиторий: **`FantomeKGZ/CoilMaster`**

Этот файл — актуальная точка продолжения после `22_WORKSHOP_REGISTRY_SCALING_2026-08-12.md`. Он дополняет handoff `14–22` и исправляет устаревший промежуточный статус workshop list compatibility mode.

## 1. Source-of-truth и стиль работы

- Единственный source of truth для исходников: **`cmp-protocol-v1`**.
- `main` для исходников **не использовать**.
- Перед каждым изменением существующего файла сначала fetch текущей версии именно из `cmp-protocol-v1` и использовать текущий blob SHA.
- Для нового файла сначала проверить, что path отсутствует.
- При конфликте/409 заново fetch актуальный blob и объединить изменения.
- Работать сразу кодом и коммитами; не заменять реализацию длинным планированием и не просить пользователя вручную подтверждать каждый commit.
- Не утверждать `CI/build green`, если это не подтверждено фактическим результатом.
- Hardware E2E не считать доказанным repo-review, если пользователь не подтвердил его на стенде.

## 2. Safety invariants — не менять

```text
NO automatic physical START
NO auto-resume after reboot
ESP32/Web never controls SSR directly
RUN_COMPLETED alone never writes off wire
wire writeoff remains manual
wire writeoff bound to exact spool_id + source_session_id + source_run_id
```

Основной production flow:

```text
client
→ motor
→ OPEN repair
→ costing
→ linked winding
→ exact spool
→ immutable snapshot + spool selection
→ UART
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ manual exact-run wire writeoff
→ costing
→ finalization preflight
→ CLOSED
→ reports
→ read-only backup
```

## 3. Hardware status, который уже подтверждён

### Linked production E2E

На реальном стенде подтверждён полный happy path:

```text
client / motor / OPEN repair
→ linked winding
→ exact spool selection
→ UART delivery
→ physical START
→ real winding
→ RUN_STARTED / RUN_COMPLETED
→ manual exact-run wire writeoff
→ writeoff data visible in UI
→ costing / finalization
→ CLOSED / final data found
→ backup/manifest readable
```

См. `15_HARDWARE_PRODUCTION_E2E_2026-08-12.md`.

### Backup during active winding

Hardware negative test подтверждён:

```text
active RUN_STARTED
→ GET /api/backup/manifest
→ export blocked
→ deep stability audit not started
```

См. `16_HARDWARE_NEGATIVE_BACKUP_ACTIVE_2026-08-12.md`.

### Standalone Arduino flow

Ранее пользователь отдельно подтвердил работу standalone/local Arduino task creation и отправки `LOCAL_EVT` на ESP32.

### Last confirmed ESP32 build

Последний подтверждённый пользователем ESP32 clean build был **до текущих recovery/archive/workshop scaling изменений**:

```text
RAM:   14.4% (47320 / 327680 bytes)
Flash: 86.7% (примерно 1.136 MB / 1.310 MB)
SUCCESS
```

После него код существенно менялся. Поэтому для текущего code HEAD:

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

## 4. Production-hardening sequence, уже выполненная в repo

Подробности сохранены в:

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
```

Ключевые уже закрытые результаты:

- allocator revalidates persisted high-water before allocation;
- dangling allocator temp / malformed state fail closed;
- backup restore не ротирует corrupt main обратно в authoritative backup;
- missing main + valid backup восстанавливает high-water;
- runtime state creation rechecks latest persisted job and terminal state;
- unexpected/temp state files block recovery;
- `TIMED_OUT` считается ambiguous и требует `MANUAL_REVIEW_REQUIRED`;
- timeout не делает `arduino_online=true`;
- exact duplicate UART events остаются idempotent, conflicting replay rejected;
- operator recovery revalidates exact snapshot and linked spool selection;
- backup guard revalidates persisted immutable identity;
- autonomous deep audit больше не квадратичный;
- autonomous HTTP archive bounded by cursor pages;
- autonomous boot uses authoritative `validateStorage()`;
- normal `LOCAL_EVT` append/replay uses bounded tail lookup;
- non-empty NDJSON must terminate with `\n`, interrupted append fails closed.

## 5. Autonomous archive scaling — current state

Authoritative files:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

HTTP:

```text
GET /api/autonomous-windings
limit default 20
limit max 32
cursor opaque byte offset
has_more
next_cursor
```

Cursor does not split a normal `RUN_STARTED + RUN_COMPLETED` logical task.

Boot/deep audit uses authoritative bounded-complexity `validateStorage()`.
Normal append/replay reads bounded tail rather than full history.
No DB migration, destructive compaction or automatic rotation has been introduced.

## 6. Workshop registry scaling — original checkpoint from doc 22

Authoritative files:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
/data/repairs/pricing.ndjson
```

Already added before the latest migration:

```text
RepairRegistry::appendClientsPageJson()
RepairRegistry::appendMotorsPageJson()
RepairRegistry::appendRepairsPageJson()
```

Page contract:

```text
limit default 20
limit max 32
cursor opaque byte offset
has_more
next_cursor
max_page_size
```

Cursor must be an NDJSON record boundary.
Existing filters are preserved:

```text
clients: phone
motors: q / name
repairs: client_id
```

Exact read-only endpoints:

```text
GET /api/clients/by-id?client_id=...
GET /api/motors/by-id?motor_id=...
GET /api/repairs/by-id?repair_id=...
```

Exact lookup fails closed on duplicate matching identity instead of silently selecting one row.

Business-data audit and boot were already changed from old nested/O(n²) validation to authoritative bounded-complexity passes/batches. `RepairRegistry::begin()` uses `BackupBusinessDataIntegrityAudit::checkWorkshopRegistry()`. `nextId()` uses one strict monotonic pass.

## 7. Latest workshop consumer migration — completed after doc 22

The temporary compatibility note in docs 06/22 is now obsolete.

### Desktop costing

`firmware/esp32/web/desktop/costing.html` now loads one repair by exact `repair_id`, then only its linked `client_id` and `motor_id` by exact lookup.

Commit:

```text
29eb1dfe88d6951aa12a88a8118ac0b2c8b00db3
Use exact registry lookups in desktop costing
```

Mobile costing was already exact before this block.

### Linked winding screens

Both linked winding screens now use exact registry lookup:

```text
7c0577ace26bf5cbeea848b9ef910f72c4f803b3
Use exact registry lookups for desktop linked winding

4de767ea4be6301a6c662c2a05fa5673ae4ec051
Use exact registry lookups for mobile linked winding
```

They no longer enumerate the full repair/motor registries just to build one production linkage.
Safety checks around exact spool, immutable linkage and physical START remain unchanged.

### Reports

Both reports now scan repair registry in bounded pages of 32 and perform exact client/motor lookup only for repairs in the selected month:

```text
6b2bde50aabf8e7e47c55d58dbcb5d9dce5735f3
Page workshop registry in desktop reports

cf92d312a9ad66de551e44bde02d81646a9c9b4b
Page workshop registry in mobile reports
```

Financial aggregation remains fail-closed: if any repair/costing/lookup is not trustworthy, global financial totals are not presented as confirmed.

### Wire writeoff lifecycle checks

Both writeoff pages now verify one repair via exact `/api/repairs/by-id` instead of reading the whole repair registry:

```text
036c05fe31d6d3a02b71ec6bd7dfeac399da3c8a
Use exact repair lookup in desktop writeoff

6a60c6545f4ef5b0923e9fac406c3c05086d0158
Use exact repair lookup in mobile writeoff
```

Exact-run writeoff safety semantics were not changed.

### Additional materials lifecycle checks

Both materials pages now verify one repair via exact lookup:

```text
3e640eb5b0fd3ac40bed67a896c61f070334f3c3
Use exact repair lookup in desktop materials

42963b1077875bfa0f1e548266d078d386a187ff
Use exact repair lookup in mobile materials
```

### Screens reviewed but not needing migration

- pricing-audit already uses scoped costing/history APIs and did not depend on full workshop registries;
- main/index path is status-oriented and did not require a workshop full-list compatibility request.

## 8. Workshop list API is now bounded by default

Commit:

```text
a6a136d0ed595825441b58faedae932dd89b1586
Make workshop list paging mandatory
```

Current runtime semantics:

```text
GET /api/clients
GET /api/motors
GET /api/repairs
```

Even with **no `cursor` or `limit` arguments**, each endpoint now returns a bounded page:

```text
cursor = 0
limit = 20
max limit = 32
```

Responses include page metadata (`count`, `limit`, `cursor`, `has_more`, `next_cursor`, `max_page_size`).

The previous branch that returned one unbounded JSON response when paging args were absent has been removed.

## 9. Legacy unbounded formatter/readers removed

Header cleanup:

```text
e81f34c98600393b160f5f1b0f9c0234e3031498
Remove legacy unbounded registry formatters
```

Removed declarations:

```text
appendClientsJson()
appendMotorsJson()
appendRepairsJson()
```

Source cleanup:

```text
a4282018b5deeaa989494ec46f57975f8f47edad
Remove legacy unbounded registry readers
```

Removed implementations of the same three full-file response formatters from `CM_RepairRegistry.cpp`.

`appendSimilarMotorsJson()` remains intentionally: it is a separate similarity API, not the removed general full-registry list compatibility path.

### Code HEAD immediately before this handoff file

```text
a4282018b5deeaa989494ec46f57975f8f47edad
Remove legacy unbounded registry readers
```

Current relevant blobs at that checkpoint:

```text
firmware/esp32/src/CM_RepairRegistry.cpp
  54047c9cfa09b3fb4e548aead7b9e9d2cd0a552b

firmware/esp32/src/CM_RepairRegistry.h
  6c951cee3adf3e0bda9314a6a65492d6f84bb5c5

firmware/esp32/src/CM_RepairRegistryWeb.cpp
  4abaa7f94deda0b1bea0ddc651e6fb1c1f8f961a

firmware/esp32/src/CM_RepairRegistryPage.cpp
  321ebd006349776aa64a78d0d9e28e85b51a995a

firmware/esp32/src/CM_RepairRegistryLookup.cpp
  ef0ab0f4d9a2fe7d6ec847f6187fb4f1931e13a5

firmware/esp32/src/CM_RepairRegistrySimilarity.cpp
  db6ab3ab4701f05f5541870e211b5fe0affaa1b0
```

## 10. What was being done when the chat stopped

The consumer migration and backend removal are complete in repo.

The active task was **compile-safety audit after deleting the legacy registry readers**:

1. Verify that declarations in `CM_RepairRegistry.h` match definitions across:
   - `CM_RepairRegistryPage.cpp`
   - `CM_RepairRegistryLookup.cpp`
   - `CM_RepairRegistrySimilarity.cpp`
   - `CM_RepairRegistry.cpp`
2. Verify there are no remaining references to removed:
   - `appendClientsJson()`
   - `appendMotorsJson()`
   - `appendRepairsJson()`
3. Re-check cursor/page semantics, especially:
   - default `cursor=0`, `limit=20`;
   - max 32;
   - invalid cursor not on record boundary fails closed;
   - filters continue correctly across pages;
   - `has_more/next_cursor` progress cannot loop.
4. Then run a **clean ESP32 build**.

Do not claim build success before user returns a real result.

## 11. Immediate next commands after repo compile-safety review

On the user's Windows/PlatformIO environment:

```powershell
pio run -e esp32 -t clean
pio run -e esp32
```

Only if build is successful:

```powershell
pio run -e esp32 -t upload
```

Because web files changed, after successful firmware upload completely replace microSD `/web` with current repository `firmware/esp32/web`. Do **not** overlay onto an old directory.

## 12. Runtime checks after successful build/upload

Minimum workshop scaling smoke test:

```text
/api/clients
/api/motors
/api/repairs
```

Each request without paging args should now still return only the first bounded page (default 20) with page metadata.

Then verify:

```text
/api/clients/by-id?client_id=<known>
/api/motors/by-id?motor_id=<known>
/api/repairs/by-id?repair_id=<known>
```

UI checks:

- clients list/search;
- motors list/search;
- repairs list;
- desktop + mobile costing;
- desktop + mobile linked winding context;
- desktop + mobile reports;
- desktop + mobile writeoff lifecycle;
- desktop + mobile materials lifecycle.

No new physical winding is required just to verify list pagination, but production safety behaviour must remain unchanged.

## 13. Performance follow-up after build/runtime verification

Do not introduce DB migration or arbitrary rotation yet.

Next reviewable performance item:

`appendRepairsPageJson()` decorates each returned repair row with current status, and current `repairClosed()` can scan the status ledger per row. Since page size is bounded to 32, HTTP response RAM is bounded, but status lookup I/O may still grow with history.

Only optimize this after current batch compiles/runs. Preferred direction is a bounded/batched status resolution for one page rather than changing storage format prematurely.

After that, collect populated dataset metrics through `/api/backup/manifest` and base any segmentation/rotation threshold on actual file size/count/latency.

## 14. Important historical notes not to regress

- Arduino local standalone archive path already works on hardware.
- Exact spool/session/run writeoff safety is production-critical.
- `RUN_COMPLETED` never directly deducts wire.
- `TIMED_OUT` remains manual-review-required because Arduino acceptance may be ambiguous.
- Backup during active/ambiguous state must stay blocked.
- Recovery closure must verify immutable snapshot and linked spool selection.
- No automatic START and no auto-resume.

## 15. Short continuation instruction

Start from current `cmp-protocol-v1`, read this file plus `06_ACTIVE_WORK_AND_NEXT_STEPS.md` and docs `15–22`, then continue **without planning detour** from compile-safety of the mandatory workshop paging cleanup. Fetch current files/blobs before edits. First close compile-safety/no-stale-reference review; then request/perform clean ESP32 build. Do not claim CI/build/hardware success without evidence.
