# CoilMaster — final new-chat handoff — 2026-08-12

Repository: `FantomeKGZ/CoilMaster`  
Only source-of-truth branch: `cmp-protocol-v1`  
**Never use `main` as implementation source.**

This file is the final continuation checkpoint for moving the current work into a new ChatGPT conversation. It consolidates the current process, safety invariants, confirmed hardware checkpoints, archive/workshop scaling work, and the exact next action.

The latest known code checkpoint immediately before the chat-handoff documentation was:

```text
a4282018b5deeaa989494ec46f57975f8f47edad
Remove legacy unbounded registry readers
```

Always fetch the current `cmp-protocol-v1` branch before editing because documentation commits may exist after that code checkpoint.

---

## 1. Required working style

The user wants direct implementation + commits, not long planning.

Before modifying any existing file:

1. fetch the exact current file from `cmp-protocol-v1`;
2. use the current blob SHA for the write;
3. on conflict/409, re-fetch and merge against the new current file;
4. for a new file, first verify the path is absent (404);
5. do not claim CI/build success unless actually confirmed;
6. user-local PlatformIO `SUCCESS` means `LOCAL BUILD CONFIRMED`, not CI;
7. hardware E2E cannot be inferred from repo review alone.

Do not ask the user to manually verify every commit. Move through code directly, then request only the concrete build/hardware evidence that cannot be obtained from the repository.

---

## 2. Safety invariants — never change

- No automatic physical START.
- No auto-resume after reboot.
- ESP32/Web never directly controls SSR.
- `RUN_COMPLETED` alone never writes off/deducts wire.
- Wire writeoff remains manual and tied to the exact:
  - `spool_id`
  - `source_session_id`
  - `source_run_id`

These are architecture boundaries, not temporary implementation choices.

---

## 3. Production flow

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
→ reports/archive
→ read-only backup
```

---

## 4. Hardware checkpoints already confirmed

### 4.1 Arduino standalone / autonomous path

Real hardware already confirmed:

```text
Standalone Arduino local program
→ physical START
→ real winding
→ LOCAL_EVT
→ ESP32 autonomous archive
```

Earlier user confirmation also established local Arduino program creation and sending to ESP32 as working.

### 4.2 Full linked production happy path

The real linked flow has been hardware-validated:

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

Therefore the main production happy-path hardware E2E is closed unless a regression is later observed.

### 4.3 Negative active-backup case

Hardware also confirmed the negative case:

```text
active RUN_STARTED
→ backup request
→ export blocked
→ deep stability scan not started
```

Do not re-open this as unverified without contrary hardware evidence.

---

## 5. Backup / integrity state

Important project handoffs under `docs/PROJECT_HANDOFF`:

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

Important established fail-closed behavior:

```text
TIMED_OUT → MANUAL_REVIEW_REQUIRED
manual-review / ambiguous persisted state → backup blocked
linked recovery → exact snapshot + exact immutable spool-selection
wrong spool/session/run → writeoff rejected before warehouse mutation
RUN_COMPLETED alone → no automatic wire writeoff
```

Earlier real-hardware `/api/backup/manifest` baseline confirmed:

```text
read_only=true
arbitrary_paths_allowed=false
export_allowed=true
activity_state_verified=true
snapshot_stability_checked=true
snapshot_stable=true
snapshot_stability_reason=null
snapshot_stability_duration_ms=1429
```

Approximate nearly-empty dataset audit baseline:

```text
material persistence             634 ms
winding session persistence      376 ms
session directory scan            93 ms
persistent ID audit               74 ms
business data audit               58 ms
winding persistence               55 ms
autonomous archive audit          34 ms
warehouse persistence             25 ms
conductor settings                23 ms
```

These small-dataset timings are only a baseline. Do not invent database/rotation/segmentation thresholds from them.

---

## 6. Autonomous Arduino archive

Authoritative append-only files:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

UART local event format:

```text
CMP1|LOCAL_EVT|<RUN_STARTED/RUN_COMPLETED>|<session>|<run>|<completed>|<WORKING/STARTING>|<coil_count>|<comma-separated-turns>|<CRC>
```

Remote/web events remain:

```text
CMP1|EVT|...
```

Semantic validation on ESP32:

```text
RUN_STARTED   => completed_runs == 0
RUN_COMPLETED => completed_runs > 0
```

Current archive hardening/scaling includes:

- bounded HTTP paging;
- default page size 20;
- maximum page size 32;
- opaque byte-offset cursor;
- `has_more` / `next_cursor`;
- cursor does not split a normal STARTED/COMPLETED pair;
- authoritative boot-time archive validation;
- assignment references audited in fixed-size batches;
- runtime LOCAL_EVT replay/completion uses bounded tail state instead of repeatedly scanning the entire history;
- non-empty NDJSON must end with `\n`; interrupted append fails closed;
- assignment path avoids duplicate completed-task lookup via checked assignment logic.

Current autonomous API:

```text
GET  /api/autonomous-windings
POST /api/autonomous-windings/assign
```

---

## 7. Workshop registry scaling — current code state

Authoritative workshop files:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
/data/repairs/pricing.ndjson
```

No database migration, destructive compaction, or arbitrary automatic rotation was introduced.

### 7.1 Bounded list readers

`RepairRegistry` now uses bounded page readers:

```text
appendClientsPageJson()
appendMotorsPageJson()
appendRepairsPageJson()
```

Maximum page size:

```text
32
```

Cursor is an opaque NDJSON byte offset and must land on a record boundary.

Existing filters remain:

```text
clients: phone
motors: q / name
repairs: client_id
```

### 7.2 Exact lookup endpoints

```text
GET /api/clients/by-id?client_id=...
GET /api/motors/by-id?motor_id=...
GET /api/repairs/by-id?repair_id=...
```

Exact lookup rejects duplicate matching identities rather than silently returning one.

### 7.3 Bounded-default HTTP behavior

Current `CM_RepairRegistryWeb.cpp::parsePaging()` starts with:

```text
cursor = 0
limit  = 20
```

Therefore even requests without explicit paging parameters are bounded:

```text
GET /api/clients
GET /api/motors
GET /api/repairs
```

Typical response metadata includes:

```text
items
count
limit
cursor
has_more
next_cursor
max_page_size
```

Maximum page size is 32.

### 7.4 Legacy unbounded readers removed

Removed from `RepairRegistry`:

```text
appendClientsJson()
appendMotorsJson()
appendRepairsJson()
```

The separate similarity path remains because it is not the old full-registry list formatter.

Known current blobs at the code checkpoint:

```text
firmware/esp32/src/CM_RepairRegistryWeb.cpp
4abaa7f94deda0b1bea0ddc651e6fb1c1f8f961a

firmware/esp32/src/CM_RepairRegistry.h
6c951cee3adf3e0bda9314a6a65492d6f84bb5c5

firmware/esp32/src/CM_RepairRegistry.cpp
54047c9cfa09b3fb4e548aead7b9e9d2cd0a552b

firmware/esp32/src/CM_RepairRegistryPage.cpp
321ebd006349776aa64a78d0d9e28e85b51a995a

firmware/esp32/src/CM_RepairRegistryLookup.cpp
ef0ab0f4d9a2fe7d6ec847f6187fb4f1931e13a5

firmware/esp32/src/CM_RepairRegistryLookupWeb.cpp
382750ef2ed2561a43291592b560cfa7bd152a02
```

Treat these as checkpoint references only. Always re-fetch current blobs before edits.

---

## 8. Legacy consumer migration completed in the latest work session

Known full-workshop-list consumers were migrated before bounded-default behavior became mandatory.

Completed:

- desktop costing → exact `repair_id`, then exact linked `client_id` and `motor_id`;
- mobile costing → already exact from earlier work;
- desktop winding-job → exact linked repair/motor reads;
- mobile winding-job → exact linked repair/motor reads;
- reports → repairs aggregated through bounded pages of 32; client/motor fetched exact for relevant repairs;
- desktop writeoff → exact repair lookup;
- mobile writeoff → exact repair lookup;
- desktop materials → exact repair lookup for OPEN/CLOSED state;
- mobile materials → exact repair lookup;
- pricing-audit → reviewed; already uses scoped costing/history APIs;
- main/index → reviewed as status-only, not a legacy full-registry consumer.

After those migrations the backend default was switched to bounded paging and dead unbounded formatter code was removed.

---

## 9. Workshop boot / integrity scaling

The old O(n²)-style workshop validation path was replaced by bounded-complexity authoritative audit logic:

```text
client IDs      strict monotonic append-only pass
motor IDs       strict monotonic pass + coil_program validation
repair IDs      strict monotonic pass
repair refs     batches of 32
CLOSED status   batches of 32 + exact-one status occurrence
pricing refs    batches of 32
```

`RepairRegistry::begin()` uses the authoritative workshop/business integrity audit.

`nextId()` performs one monotonic pass and returns `last_id + 1` rather than repeatedly scanning for duplicates.

Non-empty NDJSON must end with newline; malformed/interrupted storage fails closed.

---

## 10. Current remaining workshop performance risk

`RepairRegistry::repairClosed()` still scans the repair-status ledger for a single repair state.

`appendRepairsPageJson()` bounds the number of repair records but can still trigger repeated status-ledger scans across rows.

This is the next plausible workshop scaling candidate **after the current batch builds and is smoke-tested**.

Potential later direction:

- bounded/batched status resolution for one repairs page;
- preserve exact-one CLOSED occurrence semantics;
- preserve fail-closed behavior;
- do not introduce a database or destructive compaction prematurely.

---

## 11. Arduino / Uno hardware state

Earlier Arduino SRAM/reset-loop hardening was hardware-confirmed. Successful startup reached:

```text
CM_BOOT stage=LCD_RENDERED
CM_BOOT stage=OUTPUTS
CM_BOOT stage=READY
CM_ALIVE ... free_sram≈366
```

Uno SRAM remains tight. Avoid careless large local buffers.

Known physical pin context:

```text
D11 — Buzzer
D12 — SSR
A0  — Hall
A1  — TX Arduino → ESP32
A2  — RX Arduino ← ESP32
```

The former continuous buzzer symptom was a wiring error: buzzer had been plugged into D12 instead of D11. Do not change buzzer logic unless a new symptom remains with correct wiring.

---

## 12. PlatformIO context

Clean build commands:

```powershell
pio run -e uno -t clean
pio run -e uno
pio run -e esp32 -t clean
pio run -e esp32
```

Upload only after relevant build success:

```powershell
pio run -e esp32 -t upload
pio run -e uno -t upload
```

Serial monitor: `115200`.

ESP32 web assets live on microSD under `/web`. Because multiple `firmware/esp32/web/...` files changed in the scaling work, replace the whole `/web` folder with the current repo version after flashing; do not overlay a stale web folder.

Latest previously confirmed ESP32 build predates the newest archive/workshop scaling batch:

```text
RAM:   14.4% (~47,320 / 327,680 bytes)
Flash: 86.7% (~1,136,229 / 1,310,720 bytes)
SUCCESS
```

Therefore for the current branch state:

```text
CURRENT HEAD BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

---

## 13. Exact interruption point

The latest chat was interrupted while doing compile-safety review after removing the legacy full-registry formatter path.

Already established from current branch inspection:

- `CM_RepairRegistry.h` no longer declares legacy unbounded readers;
- `CM_RepairRegistryPage.cpp` exists for bounded paging;
- `CM_RepairRegistryLookup.cpp` exists for exact lookup;
- `CM_RepairRegistrySimilarity.cpp` remains for the separate similarity API;
- `CM_RepairRegistryWeb.cpp` uses bounded page semantics with default `cursor=0, limit=20`;
- `CM_RepairRegistry.cpp` no longer contains the three legacy list formatter definitions;
- the code checkpoint was `a4282018...` before handoff documentation commits.

Still not confirmed for the newest batch:

```text
ESP32 clean build
link success
current RAM usage
current Flash usage
GitHub CI
hardware regression/smoke test of bounded-default workshop API
```

---

## 14. Exact next action in a new chat

Do **not** start another large refactor first.

First sync/fetch current `cmp-protocol-v1`, then run or ask the user to run:

```powershell
pio run -e esp32 -t clean
pio run -e esp32
```

If `SUCCESS` is returned:

1. record current RAM and Flash numbers;
2. mark `LOCAL ESP32 BUILD CONFIRMED`;
3. keep `CI NOT CONFIRMED` unless CI is explicitly inspected;
4. flash ESP32;
5. replace microSD `/web` with current repo `firmware/esp32/web`;
6. smoke-test:
   - `/api/clients`
   - `/api/motors`
   - `/api/repairs`
   - all three `/by-id` endpoints
   - clients page
   - motors page
   - repairs page
   - costing
   - winding-job
   - writeoff
   - materials
   - reports
7. confirm default list responses contain at most 20 records and provide `has_more/next_cursor` when more data exists;
8. only after build + smoke-test consider batched repair-status resolution;
9. collect populated-dataset `/api/backup/manifest` timings before choosing any segmentation/rotation threshold.

If the build fails, fix compile/link errors first using fresh file fetch + current SHA. Do not move to performance refactors while the current batch is unbuildable.

---

## 15. New-chat starter text

Use the following as the first message in a new ChatGPT conversation:

```text
Продолжаем работу над CoilMaster.

Репозиторий: FantomeKGZ/CoilMaster.
Рабочая и единственная source-of-truth ветка: cmp-protocol-v1. main для исходников не использовать.

Сначала обязательно прочитай документы в docs/PROJECT_HANDOFF, особенно:
- 14_HARDWARE_MANIFEST_BASELINE_2026-08-12.md
- 15_HARDWARE_PRODUCTION_E2E_2026-08-12.md
- 16_HARDWARE_NEGATIVE_BACKUP_ACTIVE_2026-08-12.md
- 17_RUNTIME_STORAGE_CORRUPTION_HARDENING_2026-08-12.md
- 18_UART_TIMEOUT_REPLAY_HARDENING_2026-08-12.md
- 19_CONTROLLED_RECOVERY_AND_ARCHIVE_SCALING_2026-08-12.md
- 20_AUTONOMOUS_ARCHIVE_PAGING_2026-08-12.md
- 21_AUTONOMOUS_ARCHIVE_BOOT_AND_APPEND_SCALING_2026-08-12.md
- 22_WORKSHOP_REGISTRY_SCALING_2026-08-12.md
- 23_CHAT_CONTINUATION_HANDOFF_2026-08-12.md
- 24_NEW_CHAT_HANDOFF_2026-08-12.md

Работаем дальше сразу кодом и коммитами, без долгого планирования и без просьб вручную проверять каждый commit. Перед каждым изменением существующего файла сначала fetch актуальной версии именно из cmp-protocol-v1 и используй текущий blob SHA. Для нового файла сначала проверить, что пути нет. Если SHA изменился — re-fetch и merge. Не утверждай, что CI/build зелёный, если это не подтверждено.

Safety-инварианты не менять:
- никакого automatic physical START;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
- RUN_COMPLETED сам по себе не списывает провод;
- списание провода остаётся ручным и связано с exact spool_id + source_session_id + source_run_id.

Основной production flow уже hardware-validated:
client → motor → OPEN repair → costing → linked winding → exact spool → immutable snapshot/spool selection → UART → physical START → RUN_STARTED/RUN_COMPLETED → manual exact-run wire writeoff → costing → finalization preflight → CLOSED → reports/archive → stable read-only backup.

Также hardware-confirmed negative backup case: при активном RUN_STARTED backup/export блокируется до запуска deep stability scan.

Последний большой repo-side блок — scaling автономного архива и workshop registry без миграции в БД:
- autonomous archive bounded paging, default 20/max 32, opaque byte-offset cursor, integrity/boot/append hardening;
- workshop clients/motors/repairs bounded paging;
- exact endpoints /api/clients/by-id, /api/motors/by-id, /api/repairs/by-id;
- все найденные legacy UI consumers переведены на exact lookup или bounded pages;
- GET /api/clients|motors|repairs теперь bounded по умолчанию: cursor=0, limit=20;
- legacy RepairRegistry::appendClientsJson / appendMotorsJson / appendRepairsJson удалены;
- последний известный code checkpoint перед handoff docs: a4282018b5deeaa989494ec46f57975f8f47edad (Remove legacy unbounded registry readers).

Текущий batch ПОСЛЕ этих изменений ещё не подтверждён clean build. CI также не подтверждён.

Поэтому начни не с нового рефакторинга, а с compile-safety/current-state audit и clean ESP32 build checkpoint. Команды локально:
pio run -e esp32 -t clean
pio run -e esp32

После SUCCESS нужно записать RAM/Flash, прошить ESP32, заменить целиком /web на microSD из текущего firmware/esp32/web и smoke-test bounded list + exact endpoints и страницы clients/motors/repairs/costing/winding-job/writeoff/materials/reports.

Только после успешного build/smoke-test переходить к следующему scaling кандидату: repairClosed()/repair-status lookup внутри paged repairs, желательно bounded/batched и строго fail-closed. Никаких преждевременных DB/rotation thresholds без populated-dataset measurements.
```

## 16. Post-handoff continuation — batched repair status resolution

The workshop scaling candidate recorded in section 10 has now been implemented and compile-tested.

```text
6a64bf66045281bf2f37e8f9d7ad2250205f1369  Batch repair status resolution for paged registry
52b6e2a69664bc150a51978b292b437079bd1933  Resolve paged repair statuses in one scan
```

`appendRepairsPageJson()` collects the bounded page first and resolves every page repair against `repair-status.ndjson` in one strict forward pass. It still rejects malformed JSON, unterminated NDJSON and duplicate CLOSED evidence for a selected repair.

Verified for `52b6e2a69664bc150a51978b292b437079bd1933`:

```text
ESP32 Build: SUCCESS
CMP Protocol Tests: SUCCESS
```

The next production-hardening choice must be driven by populated-dataset backup metrics or a concrete negative/fault test. The safety invariants and storage formats remain unchanged.

## 17. Post-handoff continuation — visible collection pagination

The user identified that bounded backend pages were still being reassembled into complete browser catalogs. This is now corrected for the main visible growing collections.

Both mobile and desktop now keep one visible page (20 records) for clients, motors, repairs and winding history. Reports render 20 rows at a time while still reading every CLOSED repair required for a trustworthy monthly financial aggregate. Repair selectors use exact selected client/motor lookups and no longer enumerate both registries. Costing processes winding history with streaming counters rather than a full event array.

Code HEAD and verification:

```text
df632cd17aec82af6861fcdcf552a3ef90e20224
ESP32 Build: SUCCESS
CMP Protocol Tests: SUCCESS
```

The next confirmed scaling block is storage/API pagination for materials, material usage/adjustments, warehouse spools and warehouse writeoff history. Pagination must be implemented server-side before UI migration; existing exact provenance, integrity and safety rules remain unchanged.


## 18. Materials and warehouse pagination completed

The remaining confirmed scaling block is closed. These endpoints now return bounded cursor pages with a default of 20 and hard maximum of 32:

```text
GET /api/materials
GET /api/materials/adjustments
GET /api/materials/usage
GET /api/warehouse/spools
GET /api/warehouse/write-offs
```

Desktop and mobile now page the material catalogue, material adjustment history, repair material selector, warehouse spool catalogue, write-off spool selector and repair write-off history. Material usage history is bounded at the API/storage boundary for its existing consumers.

For write-off history, full server totals remain authoritative across the complete repair history; only the visible operation rows are paged. The safety contract is unchanged: no automatic write-off, no physical START from ESP32/Web, and manual write-off still requires the exact persisted `spool_id + source_session_id + source_run_id`.

Verified final code checkpoint:

```text
53285422a646b488776013029a438f24faeabfc6
ESP32 Build: SUCCESS
CMP Protocol Tests: SUCCESS
```

Next work should be chosen from populated-device timing/RAM measurements, hardware smoke testing, or a concrete negative/fault result. Do not reopen already bounded registries without measured evidence.


## 2026-08-15 addendum: current percentage and importer checkpoint

Current CoilMaster v1 completion estimate is **91%**. The weighted basis and
remaining work are recorded in `01_CURRENT_STATE.md` and
`06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

The motor/winding JSON import path was re-audited and hardened in
`684e848c235b5f37607e9ca814e8bc11647c1b5d`. Unknown fields, invalid source
dates/URLs, provenance inconsistencies, package-internal duplicates and
same-preview resubmission of successful rows are now blocked. Firmware and both
web UIs enforce the relevant checks. CMP Protocol Tests, executable web import
audit and ESP32 Build are confirmed successful (RAM 51408 bytes, Flash 1314657
bytes). Real-device disposable-package verification remains pending.


## 2026-08-15 addendum: rollback snapshot hardware-confirmed

The user confirmed the real-device rollback-snapshot test for
`1e7af6c3031f513b97cbfa974c922cd593d02c78`: CRC32 completion, unchanged
working data, reboot `STALE` behavior and explicit cleanup all passed. No restore
was applied. The next restore block may be implemented only as an explicit
operator transaction with a freshly validated rollback snapshot; automatic
restore and reboot continuation remain prohibited.
