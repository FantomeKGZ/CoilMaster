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

Более поздние cleanup commits не устанавливают новый firmware GREEN baseline автоматически. Для `06a752663504d58ca6908414f8aa8786007c6877` и текущего HEAD `5e704cade4977a2707609be8a35ee8f49f1a8afa` connector вернул 0 связанных workflow runs, поэтому GREEN для них не заявляется.

## Cleanup status

Full audit A–E завершён. После финального split-owner/crash-residue sweep controlled cleanup консервативно находится на **~98% complete / ~2% remaining** до software cleanup checkpoint. Hardware smoke/recovery verification — отдельный release gate и в эти 2% не входит.

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

### Warehouse duplicate bootstrap defect fixed

`main.cpp` intentionally calls both `warehouseWeb.begin()` and `warehouseWeb.beginSpoolList()`. Before the correction, `beginSpoolList()` duplicated `beginWriteOff()` and recreated/registered conductor/settings/material and repair/similarity Web services already owned elsewhere, allowing duplicate HTTP route registration.

Fixed in:

```text
06a752663504d58ca6908414f8aa8786007c6877
fix(esp32): remove duplicate warehouse web bootstrap
```

`CM_WarehouseSpoolWeb.cpp` now registers only `/api/warehouse/spools`, `/api/warehouse/spools/material` and `/api/warehouse/material-summary`. Common warehouse/write-off services remain owned by `WarehouseWeb::begin()`. No physical-control or safety boundary changed.

### Root / AI / docs cleanup

Mandatory entrypoints and high-risk thematic docs were aligned to current code/contracts:

```text
903cb7dd...  /AGENTS.md current baseline/progress
058eabc9...  root README exact-spool/current baseline
 d655e113... docs/PROJECT_HANDOFF/00_READ_FIRST.md
 e7a12104... docs/AI_AGENT/00_START_HERE.md
67ad0f55...  docs/13_WAREHOUSE.md
b54a801c...  docs/16_WINDING_SESSION_WORKFLOW.md
4004063d...  docs/08_API.md
a4253d1e...  docs/15_BUILD_AND_TEST.md
4e1a0b74...  docs/07_WEB_PORTAL.md
4593103f...  docs/04_ESP32_CORE.md
0b4a5f3f...  docs/01_SYSTEM_ARCHITECTURE.md
8a7c57bd...  docs/AI_AGENT/01_PROJECT_MAP.md buzzer pin A3
 d9bb59ce... docs/06_DATABASE.md current persistence ownership map
2cb4b3c2...  docs/00_PROJECT_VISION.md current two-controller/data model
95399017...  docs/12_ROADMAP.md current release gates rather than stale v0.x backlog
4ef8c5a1...  docs/14_DEVELOPMENT_ARCHITECTURE.md current source tree; removed references to deleted .ino/CM_Version/CHANGELOG
```

`docs/03_ARDUINO_CORE.md`, `docs/05_CMP_APPLICATION_PROTOCOL.md`, `docs/10_DIAGNOSTICS.md`, `PROJECT.manifest` and `docs/AI_AGENT/02_CHANGE_ROUTER.md` were directly re-read and remain `KEEP` without required correction in this pass.

### Tree / Web / tools / tests

- current-tree named compatibility sweep: no parallel Arduino/Core `Legacy/Migration/Compat/Deprecated/Old` owners; ESP32 only `Legacy` filename is proven live `CM_WarehouseLegacySpoolMaterial.cpp` -> `KEEP`;
- `scripts/platformio_build_id.py` directly owned by ESP32 `extra_scripts` -> `KEEP`;
- `tools/build_motor_reference.py` + `tools/check_motor_reference.py` directly owned by `motor-reference.yml` -> `KEEP`;
- `web/reference/motor-reference.json` is generated read-only reference data consumed by winding-reference UI -> `KEEP`;
- `web/sites/reference/{desktop,mobile}` is directly owned by `CM_StaticSiteServer` `/sites/reference` routing -> `KEEP`;
- root `web/index.html` is active desktop/mobile selector -> `KEEP`;
- desktop/mobile/shared trees contain no obvious `old/legacy/copy/tmp` artifacts;
- all 26 `Tests/Web/*.js` are explicitly executed by `cmp-protocol-tests.yml`; no orphan Web contract tests found;
- `Tests/Protocol/CMakeLists.txt` builds/registers all four host C++ targets, including CMP1Text; no orphan host test source found;
- `data/` is source/reference motor catalogue data, not a tracked runtime dump -> `KEEP`.

### Workflow cleanup

The two production build workflows no longer treat `main` as a push source branch:

```text
d007a42b...  Arduino Uno Build -> push only cmp-protocol-v1
d0beb03e...  ESP32 Build -> push only cmp-protocol-v1
88273be5...  CI trigger regression forbids reintroducing main and still requires cmp-protocol-v1/shared production coverage
```

Current direct fetch confirms both workflows contain only `cmp-protocol-v1` under push branches and the regression explicitly forbids `- main`.

CI status for the later cleanup batch is **not yet claimed GREEN** because no applicable run has been retrieved for current HEAD.

### Resolved split-owner / lookup REVIEW

- `CM_AutonomousWindingArchiveAssign.cpp` -> `KEEP`: split implementation of methods declared by `CM_AutonomousWindingArchive.h`;
- `CM_JobSpoolSelectionLookup.cpp` -> `KEEP`: split implementation of declared `loadReadOnly/load`, preserving fail-closed `.tmp` evidence handling;
- `CM_WarehouseWriteOffLookup.cpp` -> final `KEEP`: current header exposes only exact-run `confirmedWriteOffForSourceRun(source_session_id, source_run_id, found)`; current implementation validates `WarehouseMovementIntegrityAudit` first and resolves exact-run duplicate protection. The old session-only `confirmedWriteOffForSourceSession()` API is no longer present;
- `JobSnapshotStore .json.tmp` -> final `KEEP`: persistent ID allocation occurs before snapshot creation; authoritative `JobState CREATED` occurs only after successful final snapshot rename/verification. A crash-residue `.tmp` therefore cannot become an authoritative job, is never auto-promoted/resumed/deleted, and its already-consumed session ID is not reused. Higher-ID jobs may safely supersede this non-authoritative preparation evidence.

## Remaining ~2%

1. final consolidation of the already audited build-included owners into `DELETE / MERGE / KEEP / REVIEW`, with no deletion based only on search/name;
2. verify the warehouse bootstrap correction with applicable CI/Actions evidence when such a run exists;
3. create the software cleanup-complete checkpoint and synchronize `06`, `67` and mandatory entrypoints.

Do not restart completed `scripts/tools/Web/shared/data/tests/workflow` passes without concrete contrary evidence.

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

## Known KEEP examples

```text
CM_WarehouseMaterialCatalogue.cpp
CM_WarehouseSpoolMaterialList.cpp
CM_WarehouseLegacySpoolMaterial.cpp
CM_WarehouseWriteOffLookup.cpp
CM_MaterialHistory.cpp
CM_MaterialUsageHistory.cpp
CM_JobDisplayRecovery.*
CM_JobSpoolSelectionLookup.cpp
CM_AutonomousWindingArchiveAssign.cpp
Arduino/CM_HallCalibrationProtocol.*
Arduino/CM_HardwareControlProtocol.*
Arduino/CM_HallCalibrationService.*
Arduino/CM_HallTelemetry.*
Arduino/Config/CM_Features.h
Arduino/Config/CM_Pins.h
Arduino/Diagnostics/CM_Lcd1602CyrillicTest/CM_Lcd1602CyrillicTest.ino
PROJECT.manifest
data/motor_catalog/
scripts/
tools/
firmware/esp32/web/reference/
firmware/esp32/web/sites/reference/
```

Important lesson: empty GitHub code-search is never sufficient deletion proof. Direct owner/header/build/runtime/test checks are required.

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
