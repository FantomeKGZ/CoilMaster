# CoilMaster — chat continuation handoff — 2026-08-12

Repository: `FantomeKGZ/CoilMaster`  
Only source-of-truth branch: `cmp-protocol-v1`  
Do **not** use `main` as a source for implementation.

This file is the current continuation checkpoint for moving work to a new ChatGPT conversation.

The code checkpoint immediately before this handoff file was created:

```text
a4282018b5deeaa989494ec46f57975f8f47edad
Remove legacy unbounded registry readers
```

The handoff commit itself contains documentation only. Always fetch the current `cmp-protocol-v1` branch before making further code changes.

## 1. Required working process

The user explicitly wants direct implementation + commits, without long planning and without asking them to manually verify every commit.

Before modifying any existing file:

1. fetch the exact current file from `cmp-protocol-v1`;
2. use its current blob SHA for the update;
3. on conflict/409, re-fetch current content and merge;
4. for a new file, first verify the path is absent (404);
5. never claim CI/build is green unless it is actually confirmed;
6. hardware E2E cannot be inferred from repo review.

If build is only locally reported by the user, say `LOCAL BUILD CONFIRMED`; do not call that CI.

## 2. Safety invariants — never change

- No automatic physical START.
- No auto-resume after reboot.
- ESP32/Web never directly controls SSR.
- `RUN_COMPLETED` alone never writes off/deducts wire.
- Wire writeoff remains manual and tied to exact:
  - `spool_id`
  - `source_session_id`
  - `source_run_id`

These are architecture boundaries, not temporary implementation choices.

## 3. Production flow

The intended production flow is:

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

## 4. Hardware validation already confirmed

Current project handoff documents record that the real hardware has confirmed:

```text
Standalone Arduino local program
→ physical START
→ real winding
→ LOCAL_EVT
→ ESP32 autonomous archive
```

and the full linked production happy path:

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

Happy-path hardware E2E is therefore closed.

A negative hardware backup checkpoint is also confirmed:

```text
active RUN_STARTED
→ backup request
→ export blocked
→ deep stability scan not started
```

Do not re-open these as unverified unless new evidence shows a regression.

## 5. Backup / integrity state

Important documented checkpoints are under `docs/PROJECT_HANDOFF`:

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
23_CHAT_CONTINUATION_HANDOFF_2026-08-12.md
```

Key safety/hardening results already established:

```text
TIMED_OUT → MANUAL_REVIEW_REQUIRED
manual-review / ambiguous persisted state → backup blocked
linked recovery → exact snapshot + exact immutable spool-selection
wrong spool/session/run → writeoff rejected before warehouse mutation
RUN_COMPLETED alone → no automatic wire writeoff
```

Earlier real-hardware backup baseline from `/api/backup/manifest` was stable and exportable. At that baseline:

```text
snapshot_stability_checked=true
snapshot_stable=true
export_allowed=true
activity_state_verified=true
snapshot_stability_duration_ms=1429
```

The nearly-empty dataset baseline timings were approximately:

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

These empty/small dataset timings are only a baseline. Do not invent rotation or DB thresholds from them.

## 6. Autonomous Arduino archive

Authoritative append-only storage:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

UART local event form:

```text
CMP1|LOCAL_EVT|<RUN_STARTED/RUN_COMPLETED>|<session>|<run>|<completed>|<WORKING/STARTING>|<coil_count>|<comma-separated-turns>|<CRC>
```

Remote/web events remain `CMP1|EVT|...`.

Semantic validation on ESP32 rejects invalid completed counters:

```text
RUN_STARTED   => completed_runs == 0
RUN_COMPLETED => completed_runs > 0
```

Archive scaling/hardening currently includes:

- bounded HTTP paging;
- default page size 20;
- max page size 32;
- opaque byte-offset cursor;
- `has_more` / `next_cursor`;
- cursor does not split a normal STARTED/COMPLETED pair;
- boot uses authoritative archive validation;
- assignment references are audited in fixed-size batches;
- runtime LOCAL_EVT replay/completion uses bounded tail state instead of repeatedly scanning full history;
- non-empty NDJSON must end with `\n`; interrupted append fails closed.

Current autonomous API includes:

```text
GET  /api/autonomous-windings
POST /api/autonomous-windings/assign
```

Assignment path was optimized to avoid checking the completed task twice; `assignMotorChecked()` preserves typed not-found/integrity/storage/write semantics.

## 7. Arduino / Uno known hardware state

Arduino startup reset-loop work previously reduced SRAM pressure. Confirmed successful hardware boot reached:

```text
CM_BOOT stage=LCD_RENDERED
CM_BOOT stage=OUTPUTS
CM_BOOT stage=READY
CM_ALIVE ... free_sram≈366
```

Idle SRAM remains tight, so avoid careless large local buffers on Uno.

Known physical pin context:

```text
D11 — Buzzer
D12 — SSR
A0  — Hall
A1  — TX Arduino → ESP32
A2  — RX Arduino ← ESP32
```

The earlier continuous-buzzer issue was a wiring mistake: buzzer had been connected to D12 instead of D11. Do not rework buzzer code unless a new symptom remains with correct wiring.

## 8. PlatformIO build context

Important local commands:

```powershell
pio run -e uno -t clean
pio run -e uno
pio run -e esp32 -t clean
pio run -e esp32
```

Upload only after the relevant build succeeds:

```powershell
pio run -e esp32 -t upload
pio run -e uno -t upload
```

Serial monitor is 115200.

ESP32 web assets live on microSD under `/web`. When repo `firmware/esp32/web` changes, replace the whole `/web` folder from the current repo version rather than overlaying onto stale files.

Latest previously confirmed ESP32 build was before the newest archive/workshop scaling batch:

```text
RAM:   14.4% (~47320 bytes / 327680)
Flash: 86.7% (~1.136 MB / 1.310 MB)
SUCCESS
```

The current code HEAD has changed since that build.

Therefore at this handoff:

```text
CURRENT HEAD BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

## 9. Workshop registry scaling — current state

Authoritative files:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
/data/repairs/pricing.ndjson
```

No database migration, destructive compaction, or automatic rotation has been introduced.

### 9.1 Bounded page readers

`RepairRegistry` exposes only bounded list readers for the business registries:

```text
appendClientsPageJson()
appendMotorsPageJson()
appendRepairsPageJson()
```

`MaxListPageSize = 32`.

Cursor is an opaque NDJSON byte offset and must point to a record boundary.

Existing filters are preserved:

```text
clients: phone
motors: q / name
repairs: client_id
```

### 9.2 Exact lookup

Read-only exact endpoints exist:

```text
GET /api/clients/by-id?client_id=...
GET /api/motors/by-id?motor_id=...
GET /api/repairs/by-id?repair_id=...
```

Exact lookup rejects duplicate matching identities instead of silently returning one record.

### 9.3 Current list HTTP semantics — IMPORTANT NEWER STATE

Older docs `06_ACTIVE_WORK_AND_NEXT_STEPS.md` and `22_WORKSHOP_REGISTRY_SCALING_2026-08-12.md` still contain a now-stale note saying legacy GET without `cursor/limit` is temporarily unbounded.

That is no longer true on current code.

`CM_RepairRegistryWeb.cpp::parsePaging()` now always starts with:

```text
cursor = 0
limit  = 20
```

and list handlers always call the paged readers.

Therefore even:

```text
GET /api/clients
GET /api/motors
GET /api/repairs
```

without explicit paging parameters is bounded by default and returns page metadata:

```text
items
count
limit
cursor
has_more
next_cursor
max_page_size
```

Max page size is 32.

### 9.4 Legacy unbounded registry readers removed

Latest code commit before this handoff:

```text
a4282018b5deeaa989494ec46f57975f8f47edad
Remove legacy unbounded registry readers
```

Removed from `RepairRegistry`:

```text
appendClientsJson()
appendMotorsJson()
appendRepairsJson()
```

The current `CM_RepairRegistry.h` exposes page readers, exact lookups, and the separate similarity method only. Legacy unbounded list formatter declarations are gone.

Important current blobs at this checkpoint:

```text
firmware/esp32/src/CM_RepairRegistryWeb.cpp
blob 4abaa7f94deda0b1bea0ddc651e6fb1c1f8f961a

firmware/esp32/src/CM_RepairRegistry.h
blob 6c951cee3adf3e0bda9314a6a65492d6f84bb5c5

firmware/esp32/src/CM_RepairRegistry.cpp
blob 54047c9cfa09b3fb4e548aead7b9e9d2cd0a552b

firmware/esp32/src/CM_RepairRegistryPage.cpp
blob 321ebd006349776aa64a78d0d9e28e85b51a995a

firmware/esp32/src/CM_RepairRegistryLookup.cpp
blob ef0ab0f4d9a2fe7d6ec847f6187fb4f1931e13a5

firmware/esp32/src/CM_RepairRegistryLookupWeb.cpp
blob 382750ef2ed2561a43291592b560cfa7bd152a02
```

These SHA values are historical checkpoint references only. Re-fetch before editing.

## 10. Legacy consumer migration completed in the current work session

The current session migrated the remaining known workshop full-list consumers before enabling bounded-default behavior.

Completed migrations include:

- desktop costing: exact `repair_id` then exact linked `client_id` / `motor_id`;
- mobile costing: already exact earlier;
- desktop winding-job: exact linked repair/motor reads;
- mobile winding-job: exact linked repair/motor reads;
- reports: repairs are aggregated via bounded pages (32), then client/motor are fetched exact for relevant repairs rather than loading full registries;
- desktop writeoff: exact repair lookup;
- mobile writeoff: exact repair lookup;
- desktop materials: exact repair lookup for OPEN/CLOSED state;
- mobile materials: exact repair lookup;
- pricing-audit: reviewed; it already uses scoped costing/history APIs and did not need workshop full-list migration;
- main/index status path: reviewed as status-only, not a legacy full-registry consumer.

After those consumer migrations, backend default was switched to bounded paging and the dead unbounded formatter path was removed.

## 11. Workshop boot / business-data integrity

The old O(n^2) business registry validation was replaced with bounded-complexity authoritative audit logic:

```text
client IDs      strict monotonic append-only pass
motor IDs       strict monotonic pass + coil_program validation
repair IDs      strict monotonic pass
repair refs     batches of 32
CLOSED status   batches of 32 + exact-one status occurrence
pricing refs    batches of 32
```

`RepairRegistry::begin()` uses:

```text
BackupBusinessDataIntegrityAudit::checkWorkshopRegistry()
```

`nextId()` performs one monotonic pass and returns `last_id + 1`, instead of O(n^2) duplicate scans.

Non-empty NDJSON must end with newline and malformed/interrupted storage fails closed.

## 12. Current performance risk still open

`RepairRegistry::repairClosed()` still scans the repair-status ledger to answer the state for one repair.

`appendRepairsPageJson()` limits the number of repair rows, but status resolution can still cause repeated scans of `/data/workshop/repair-status.ndjson` across rows.

This is the next plausible workshop scaling candidate, but do **not** refactor it blindly before the current batch builds successfully.

Potential direction after build: bounded/batched status resolution for a repair page while preserving exact-one CLOSED occurrence semantics and fail-closed behavior.

Do not introduce a database, destructive compaction, or arbitrary rotation threshold just to solve this without measurements.

## 13. State at interruption / exact continuation point

The chat was interrupted while performing compile-safety review after the legacy formatter removal.

Already verified from current branch:

- `CM_RepairRegistry.h` no longer declares legacy unbounded readers;
- separate files exist for page, exact lookup, and similarity implementations;
- `CM_RepairRegistryWeb.cpp` always uses bounded page semantics with default `cursor=0, limit=20`;
- `CM_RepairRegistry.cpp` no longer defines the three legacy unbounded readers;
- current branch code checkpoint was `a4282018...` before this handoff documentation commit.

Not yet confirmed after the latest changes:

```text
ESP32 clean build
link success
flash size
RAM size
GitHub CI
hardware regression test for the new bounded-default workshop API
```

## 14. Exact next action in the new chat

Do not start another large refactor first.

First fetch current `cmp-protocol-v1` and perform/ask for the clean ESP32 build:

```powershell
pio run -e esp32 -t clean
pio run -e esp32
```

If the user returns `SUCCESS`:

1. record the new RAM/Flash numbers;
2. mark local build confirmed, but keep `CI NOT CONFIRMED` unless CI was actually inspected;
3. deploy the firmware;
4. because multiple `firmware/esp32/web` files changed, replace microSD `/web` with the current repo folder;
5. smoke-test workshop endpoints:
   - `/api/clients`
   - `/api/motors`
   - `/api/repairs`
   - exact `/by-id` endpoints;
   - clients/motors/repairs pages;
   - costing;
   - winding-job;
   - writeoff;
   - materials;
   - reports;
6. confirm list endpoints return no more than 20 items by default and expose `has_more/next_cursor` when more records exist;
7. only then consider optimizing repair-status lookup within paged repair reads;
8. collect populated-dataset `/api/backup/manifest` timings before setting segmentation/rotation thresholds.

If the build fails, fix compile/link errors first using fresh file fetch + current blob SHA. Do not move on to performance refactors until the batch compiles.

## 15. New-chat instruction text

A new chat should begin by being told:

```text
Продолжаем CoilMaster.
Репозиторий FantomeKGZ/CoilMaster.
Единственная source-of-truth ветка cmp-protocol-v1, main не использовать.
Сначала прочитай docs/PROJECT_HANDOFF/23_CHAT_CONTINUATION_HANDOFF_2026-08-12.md, затем при необходимости 14–22 в той же папке.

Работай сразу по коду и коммитам, без долгого планирования и без просьб вручную проверять каждый commit. Перед изменением любого существующего файла сначала fetch его актуальную версию из cmp-protocol-v1 и используй текущий blob SHA. Для новых файлов сначала проверяй 404. Не утверждай, что build/CI зелёный, если это не подтверждено.

Safety-инварианты не менять:
- никакого automatic physical START;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
- RUN_COMPLETED сам не списывает провод;
- wire writeoff только вручную и exact spool_id + source_session_id + source_run_id.

Текущий production happy-path и negative backup-on-active hardware E2E уже подтверждены.

Последний кодовый checkpoint перед handoff: a4282018b5deeaa989494ec46f57975f8f47edad — Remove legacy unbounded registry readers.
Legacy appendClientsJson/appendMotorsJson/appendRepairsJson уже удалены.
/api/clients, /api/motors, /api/repairs теперь bounded по умолчанию: cursor=0, limit=20, max=32.
Оставшиеся legacy UI consumers уже мигрированы на exact-by-ID или bounded pages: costing, winding-job, reports, writeoff, materials; pricing-audit legacy workshop lists не использует.

Сейчас текущий HEAD после последних scaling изменений ещё НЕ имеет подтверждённого clean ESP32 build. С этого и продолжай:
pio run -e esp32 -t clean
pio run -e esp32
После SUCCESS — smoke-test bounded workshop API/UI и затем только при необходимости оптимизируй repeated repair-status scans внутри paged repairs. Не вводи DB migration/rotation thresholds без populated-dataset measurements.
```

This `23_...` file supersedes the stale legacy-consumer/default-paging notes in `06_ACTIVE_WORK_AND_NEXT_STEPS.md` and `22_WORKSHOP_REGISTRY_SCALING_2026-08-12.md`. The rest of those documents remains useful historical/context information.
