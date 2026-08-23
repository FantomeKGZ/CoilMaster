# Активная работа и следующие шаги

Дата обновления: **2026-08-23**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Current verified GREEN baseline

Последний подтверждённый CI GREEN implementation/test baseline:

```text
ad17bb7029f9f0f694fcb275ce729d0c23c8e1dd
Harden release safety contract against stale JSON assertions
CMP Protocol Tests GREEN
```

Подтверждающие Actions runs:

```text
32616937088  GREEN  checkout ad17bb7029f9f0f694fcb275ce729d0c23c8e1dd
32616970608  GREEN
32616987523  GREEN
```

Run `32616937088` прошёл CMake configure/build, все 4 host C++ tests и весь Web/Protocol contract suite, включая `Audit release safety contracts`.

Более поздние cleanup commits не устанавливают новый firmware GREEN baseline автоматически. GitHub connector в текущей сессии не предоставляет надёжный список push-triggered runs по exact SHA, поэтому для latest implementation/test batch GREEN не заявляется без фактического run result.

## Cleanup status

Full audit A–E завершён. Final split-owner/crash-residue/source consolidation также завершён. Controlled software cleanup консервативно находится на **~99% complete / ~1% remaining**: остался внешний CI evidence/checkpoint для последнего implementation fix + regression. Hardware smoke/recovery verification — отдельный release gate и в этот 1% не входит.

## Уже закрыто

### Production/safety/provenance

- obsolete Arduino parallel `.ino`, old buzzer/start-button owners and stale `CM_Version.h` removed;
- obsolete conductor settings persistence owner removed;
- generated `build/` removed and `.pio/`, `build/` ignored;
- old warehouse wire catalogue/non-paginated spool list removed;
- obsolete calculator helper/static injection removed;
- active migration/recovery owners classified/protected as `KEEP`;
- Arduino/Core state-machine audit and regressions complete;
- UART lost-ACK/timeout/late-`RUN_STARTED` semantics reviewed/hardened;
- exact session/run/spool provenance hardened;
- exact-run finalization coverage and immutable selection required;
- current KG_FIRST store/API/UI requires exact immutable `spool_id`;
- historical `UNALLOCATED` remains read/audit/recovery compatibility evidence only;
- snapshot/state/selection crash-residue policies reviewed by transaction boundary;
- release safety regression corrected and CI-verified GREEN at `ad17bb7...`.

### Latest final owner sweep

Direct build-included review closed these split-owner groups as `KEEP`:

- `RepairRegistry` core/lookup/page/search/similarity: distinct declared create/read/paging/search/similarity responsibilities;
- `WindingJournalQuery` + `WindingJournalQueryValidation`: paging/history vs authoritative full-file schema validation;
- `WindingJournalTransitionAudit`: event ordering/state-transition audit, not duplicate schema validation;
- `WindingJournalSnapshotContext`: immutable snapshot/runtime context boundary and late timeout reconciliation after persisted journal evidence;
- `RepairCosting` + `RepairCostingValidation`: cost aggregation/pricing vs authoritative repair-reference validation;
- `AutonomousWindingArchive` core/assign/integrity/page: append/replay, checked assignment, whole-archive integrity and bounded task paging are separate owners;
- `CM_WarehouseLegacySpoolMaterial.cpp`: live migration endpoint for historical spools missing `wire_type`, not dead legacy code;
- current `Core/` tree remains compact with concrete input/state/UI/turn-source owners and no parallel `Legacy/Compat/Old` owner;
- current `Arduino/` tree remains in the previously audited protocol/service structure with no new deletion candidate found in the final tree-level pass.

### Warehouse duplicate bootstrap defect fixed and protected

`main.cpp` intentionally calls both `warehouseWeb.begin()` and `warehouseWeb.beginSpoolList()`. Before the correction, `beginSpoolList()` duplicated `beginWriteOff()` and recreated/registered conductor/settings/material and repair/similarity Web services already owned elsewhere, allowing duplicate HTTP route registration.

Implementation fix:

```text
06a752663504d58ca6908414f8aa8786007c6877
fix(esp32): remove duplicate warehouse web bootstrap
```

`CM_WarehouseSpoolWeb.cpp` now registers only `/api/warehouse/spools`, `/api/warehouse/spools/material` and `/api/warehouse/material-summary`. Common warehouse/write-off services remain owned by `WarehouseWeb::begin()`. No physical-control or safety boundary changed.

Regression protection:

```text
bd64e3cc4ba92a6624aed677d98c1620c165013e
test(warehouse): guard against duplicate web bootstrap
```

Existing `Tests/Web/check_warehouse_spool_list_cleanup.js`, already executed by `cmp-protocol-tests.yml`, now rejects reintroduction of `beginWriteOff`, conductor/material/repair/similarity bootstrap into `beginSpoolList()`, while requiring the common bootstrap to remain owned by `WarehouseWeb::begin()` and both split owner calls to remain explicit in `main.cpp`.

### Root / AI / docs cleanup

Mandatory entrypoints and high-risk thematic docs were aligned to current code/contracts. Latest status synchronization includes:

```text
e52d487c...  docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md snapshot review resolution
f839dda7...  docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md snapshot review resolution
fb51c386...  docs/PROJECT_HANDOFF/00_READ_FIRST.md current 98% state before final regression
3541c268...  AGENTS.md current 98% state before final regression
```

Earlier thematic docs were also aligned to current code/contracts, including `docs/13_WAREHOUSE.md`, `16_WINDING_SESSION_WORKFLOW.md`, `08_API.md`, `15_BUILD_AND_TEST.md`, `07_WEB_PORTAL.md`, `04_ESP32_CORE.md`, `01_SYSTEM_ARCHITECTURE.md`, `06_DATABASE.md`, `00_PROJECT_VISION.md`, `12_ROADMAP.md` and `14_DEVELOPMENT_ARCHITECTURE.md`.

`docs/03_ARDUINO_CORE.md`, `docs/05_CMP_APPLICATION_PROTOCOL.md`, `docs/10_DIAGNOSTICS.md`, `PROJECT.manifest` and `docs/AI_AGENT/02_CHANGE_ROUTER.md` were directly re-read and remain `KEEP`.

### Tree / Web / tools / tests

- current-tree named compatibility sweep: no parallel Arduino/Core `Legacy/Migration/Compat/Deprecated/Old` owners; ESP32 only `Legacy` filename is proven live `CM_WarehouseLegacySpoolMaterial.cpp` -> `KEEP`;
- `scripts/platformio_build_id.py` directly owned by ESP32 `extra_scripts` -> `KEEP`;
- `tools/build_motor_reference.py` + `tools/check_motor_reference.py` directly owned by `motor-reference.yml` -> `KEEP`;
- `web/reference/motor-reference.json` is generated read-only reference data consumed by winding-reference UI -> `KEEP`;
- `web/sites/reference/{desktop,mobile}` is directly owned by `CM_StaticSiteServer` `/sites/reference` routing -> `KEEP`;
- root `web/index.html` is active desktop/mobile selector -> `KEEP`;
- desktop/mobile/shared trees contain no obvious `old/legacy/copy/tmp` artifacts;
- all existing `Tests/Web/*.js` are explicitly owned by `cmp-protocol-tests.yml`; warehouse bootstrap regression was added to an already executed test rather than creating an orphan test;
- `Tests/Protocol/CMakeLists.txt` builds/registers all four host C++ targets, including CMP1Text; no orphan host test source found;
- `data/` is source/reference motor catalogue data, not a tracked runtime dump -> `KEEP`.

### Workflow cleanup

The two production build workflows no longer treat `main` as a push source branch:

```text
d007a42b...  Arduino Uno Build -> push only cmp-protocol-v1
d0beb03e...  ESP32 Build -> push only cmp-protocol-v1
88273be5...  CI trigger regression forbids reintroducing main and still requires cmp-protocol-v1/shared production coverage
```

`cmp-protocol-tests.yml` has an unconditional push trigger on `cmp-protocol-v1`, so the regression commit is in workflow scope. A concrete run result still must be observed before calling the latest batch GREEN.

### Resolved split-owner / lookup REVIEW

- `CM_AutonomousWindingArchiveAssign.cpp` -> `KEEP`: split implementation of methods declared by `CM_AutonomousWindingArchive.h`;
- `CM_JobSpoolSelectionLookup.cpp` -> `KEEP`: split implementation of declared `loadReadOnly/load`, preserving fail-closed `.tmp` evidence handling;
- `CM_WarehouseWriteOffLookup.cpp` -> final `KEEP`: current header exposes only exact-run `confirmedWriteOffForSourceRun(source_session_id, source_run_id, found)`; current implementation validates `WarehouseMovementIntegrityAudit` first and resolves exact-run duplicate protection;
- `JobSnapshotStore .json.tmp` -> final `KEEP`: persistent ID allocation occurs before snapshot creation; authoritative `JobState CREATED` occurs only after successful final snapshot rename/verification. A crash-residue `.tmp` therefore cannot become an authoritative job, is never auto-promoted/resumed/deleted, and its already-consumed session ID is not reused. Higher-ID jobs may safely supersede this non-authoritative preparation evidence.

## Final source-side classification

```text
DELETE  no new candidates remain from the final owner sweep
MERGE   no duplicate authoritative owners remain from the final owner sweep
KEEP    all reviewed live production/build/test/docs/recovery owners listed above
REVIEW  none remain from the named split-owner/crash-residue queue
FIXED   warehouse duplicate Web bootstrap (06a7526...) + regression (bd64e3c...)
```

No deletion was based only on filename or empty GitHub search.

## Remaining ~1%

1. obtain an applicable successful CI/Actions result for the latest implementation/test batch;
2. once verified, record the exact run/SHA as the new verification baseline and mark the software cleanup checkpoint complete;
3. hardware two-board smoke remains a separate release gate and does not reopen software cleanup.

## Current production material rule

For every new linked-production manual wire writeoff:

```text
source_session_id + source_run_id + exact immutable spool_id
```

Historical `UNALLOCATED` records are immutable compatibility evidence only. Never recreate optional spool as a post-`RUN_COMPLETED` fallback.

## Crash-residue classification

```text
JobStateStore .tmp/.bak
  KEEP fail-closed

JobSpoolSelectionStore .json.tmp
  KEEP bounded recovery before UART boundary

JobSnapshotStore .json.tmp
  KEEP non-authoritative preparation crash evidence; no auto-promote/resume/delete
```

Do not force these stores into one recovery policy; their durable transaction boundaries differ.

## Safety boundary — never weaken

- no automatic physical START;
- no automatic START between repeat runs;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout alone never proves Arduino idle;
- final repeat cannot automatically reopen;
- `RUN_COMPLETED` never automatically deducts material;
- writeoff remains explicit/manual and exact session + run + immutable spool;
- cancellation cannot erase immutable run/history evidence;
- restore explicit/operator-only/transactional/fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion or NDJSON truncation.

## Read order for next chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Do not restart completed audit/provenance/crash-residue work unless current source gives concrete contrary evidence. Request Serial/runtime logs only when a remaining issue is genuinely hardware-only.
