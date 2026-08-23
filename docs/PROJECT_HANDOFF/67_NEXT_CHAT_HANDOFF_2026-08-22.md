# Next chat handoff — CoilMaster cleanup — 2026-08-23

## Source of truth

```text
Repository: FantomeKGZ/CoilMaster
Branch: cmp-protocol-v1
main: DO NOT use as source code
```

Before every edit/delete fetch the current file from `cmp-protocol-v1` and use its current blob SHA. Verify a path is absent before creating a new file.

## Last verified GREEN baseline

```text
ad17bb7029f9f0f694fcb275ce729d0c23c8e1dd
Harden release safety contract against stale JSON assertions
CMP Protocol Tests GREEN
```

Verified Actions:

```text
32616937088  GREEN  checkout ad17bb7029f9f0f694fcb275ce729d0c23c8e1dd
32616970608  GREEN
32616987523  GREEN
```

Run `32616937088` passed configure/build, all 4 host C++ tests and all Web/Protocol contracts including `Audit release safety contracts`.

Later documentation/workflow cleanup commits do not establish a newer firmware GREEN baseline automatically. The workflow-cleanup batch below has not yet been confirmed by an applicable push-run result.

## Cleanup progress

Controlled code/docs/tree cleanup remains conservatively **~96% complete / ~4% remaining** until the final owner sweep and cleanup-complete checkpoint are closed.

Hardware E2E is a separate release gate and is not included in this percentage.

## Current production flow

```text
client
-> motor
-> OPEN repair
-> costing
-> linked winding
-> exact immutable spool selection
-> immutable snapshot/state
-> UART JOB
-> physical START
-> RUN_STARTED / RUN_COMPLETED
-> explicit manual exact-run exact-spool wire writeoff
-> costing / finalization preflight
-> CLOSED
-> reports
-> backup
```

## Non-negotiable safety rules

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly drive SSR;
- timeout/lost ACK alone never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never automatically deducts wire;
- current linked writeoff is exact `source_session_id + source_run_id + immutable spool_id`;
- historical `UNALLOCATED` is read/audit/recovery compatibility only;
- restore is explicit/operator-only/transactional/fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion or NDJSON truncation.

## Newly closed in the latest cleanup pass

### High-level docs / AI routing

```text
2cb4b3c2...  docs/00_PROJECT_VISION.md current two-controller/data model
95399017...  docs/12_ROADMAP.md current release gates instead of stale v0.x backlog
4ef8c5a1...  docs/14_DEVELOPMENT_ARCHITECTURE.md current source tree; no deleted .ino/CM_Version/CHANGELOG paths
4593103f...  docs/04_ESP32_CORE.md current Hall HTTP/UI state
0b4a5f3f...  docs/01_SYSTEM_ARCHITECTURE.md mandatory current exact spool
8a7c57bd...  docs/AI_AGENT/01_PROJECT_MAP.md authoritative buzzer pin A3
 d9bb59ce... docs/06_DATABASE.md current persistence ownership model
```

Earlier in the same sweep `docs/13_WAREHOUSE.md`, `16_WINDING_SESSION_WORKFLOW.md`, `08_API.md`, `15_BUILD_AND_TEST.md`, `07_WEB_PORTAL.md` and mandatory entrypoints were also aligned to current source/safety contracts.

`docs/03_ARDUINO_CORE.md`, `docs/05_CMP_APPLICATION_PROTOCOL.md`, `docs/10_DIAGNOSTICS.md`, `PROJECT.manifest` and `docs/AI_AGENT/02_CHANGE_ROUTER.md` were re-read and remain `KEEP`.

### Workflow source-branch cleanup

```text
d007a42b...  Arduino Uno Build push branch -> cmp-protocol-v1 only
d0beb03e...  ESP32 Build push branch -> cmp-protocol-v1 only
88273be5...  check_ci_trigger_contracts forbids returning main while requiring cmp-protocol-v1/shared production coverage
```

Direct current fetch confirms these source contracts. Do not call this workflow batch GREEN until an actual successful applicable run is observed.

### Tree/tests/data/owner classification

- `scripts/`, `tools/`, Web shared/reference/sites and root Web selector are direct-owner reviewed `KEEP`;
- all 26 `Tests/Web/*.js` are executed by `cmp-protocol-tests.yml`;
- all 4 host C++ test targets are registered by `Tests/Protocol/CMakeLists.txt`, including CMP1Text;
- `data/motor_catalog/` is source/reference data, not runtime output -> `KEEP`;
- `CM_AutonomousWindingArchiveAssign.cpp` -> `KEEP` split implementation;
- `CM_JobSpoolSelectionLookup.cpp` -> `KEEP` split implementation preserving fail-closed temp evidence;
- `CM_WarehouseWriteOffLookup.cpp` -> final `KEEP`: only exact-run `confirmedWriteOffForSourceRun(session, run)` remains public/current; it validates warehouse movement integrity before duplicate lookup. The former session-only REVIEW item is no longer present.

Active handoff record for this pass:

```text
823b07e01d600470cb1bec5c48a7b935c12e5b5f
Record final workflow and thematic cleanup sweep
```

## Remaining cleanup sequence (~4%)

1. finish owner-by-owner sweep of remaining build-included ESP32/Arduino/Core split implementations;
2. touch lower-risk thematic docs only when current source proves a concrete stale contract;
3. produce final `DELETE / MERGE / KEEP / REVIEW` consolidation and cleanup-complete checkpoint;
4. synchronize `06`, this file and entrypoints at completion.

Do not restart completed `scripts/tools/Web/shared/data/tests/workflow` passes without concrete contrary evidence.

## Crash-residue classification

```text
JobStateStore .tmp/.bak        KEEP fail-closed
JobSpoolSelection .json.tmp    KEEP bounded recovery before UART
JobSnapshot .json.tmp          REVIEW / fail-closed resilience
```

Do not unify these merely for symmetry.

## Important KEEP examples

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

Never delete based only on an empty GitHub search result.

## Read first in the next chat

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

Continue directly with code/commits. Do not ask for broad hardware logs during source cleanup; request only the exact Serial interval when a remaining issue becomes hardware-only.
