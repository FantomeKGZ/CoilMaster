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

Later cleanup commits do not establish a newer GREEN baseline automatically. The current connector cannot reliably enumerate push-triggered runs by exact SHA, so the latest implementation/test batch remains **not yet claimed GREEN** until an applicable successful run is observed.

## Cleanup progress

Controlled source/docs/tree cleanup is now conservatively **~99% complete / ~1% remaining**. Final owner/crash-residue/source classification is complete; the remaining 1% is external CI evidence plus the final cleanup-complete verification checkpoint.

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

## Latest cleanup pass

### Final split-owner sweep

Direct build-included review closed the named remaining groups as `KEEP`:

- `RepairRegistry` core/lookup/page/search/similarity;
- `WindingJournalQuery` + `WindingJournalQueryValidation`;
- `WindingJournalTransitionAudit`;
- `WindingJournalSnapshotContext`;
- `RepairCosting` + `RepairCostingValidation`;
- `AutonomousWindingArchive` core/assign/integrity/page;
- live `CM_WarehouseLegacySpoolMaterial.cpp` migration owner;
- current `Core/` and `Arduino/` trees with no new parallel `Legacy/Compat/Old` production owners.

These are separate declared responsibilities rather than orphan duplicate implementations.

### Warehouse duplicate bootstrap defect fixed

`main.cpp` intentionally calls both:

```text
warehouseWeb.begin();
warehouseWeb.beginSpoolList();
```

Before the fix, `beginSpoolList()` also registered `beginWriteOff()` and recreated conductor/settings/material and repair/similarity Web services already owned elsewhere, so the same HTTP routes could be registered more than once.

Implementation fix:

```text
06a752663504d58ca6908414f8aa8786007c6877
fix(esp32): remove duplicate warehouse web bootstrap
```

Current `CM_WarehouseSpoolWeb.cpp` registers only:

```text
GET  /api/warehouse/spools
POST /api/warehouse/spools/material
GET  /api/warehouse/material-summary
```

Common warehouse/write-off/conductor/material bootstrap remains owned by `WarehouseWeb::begin()`. No physical-control or safety boundary changed.

Regression protection:

```text
bd64e3cc4ba92a6624aed677d98c1620c165013e
test(warehouse): guard against duplicate web bootstrap
```

Existing `Tests/Web/check_warehouse_spool_list_cleanup.js`, already executed by `cmp-protocol-tests.yml`, now:

- rejects `beginWriteOff()` or conductor/material/repair/similarity service bootstrap inside `CM_WarehouseSpoolWeb.cpp`;
- requires common warehouse services to remain owned by `CM_WarehouseWeb.cpp`;
- requires both `warehouseWeb.begin()` and `warehouseWeb.beginSpoolList()` to remain explicit in `main.cpp`.

The test commit is under `Tests/Web/**`; `cmp-protocol-tests.yml` has an unconditional push trigger on `cmp-protocol-v1`. A concrete successful run still must be observed before calling the latest batch GREEN.

### JobSnapshot crash-residue REVIEW resolved

`JobSnapshotStore .json.tmp` is final `KEEP` as non-authoritative preparation crash evidence.

Durable order:

```text
persistent job/session ID allocation
-> JobSnapshot temp write + verify
-> rename to final immutable snapshot + verify
-> JobState CREATED
-> exact spool selection / DELIVERING / UART boundary later
```

A leftover `.json.tmp` therefore cannot become an authoritative job, its already allocated session ID is not reused, and it is never auto-promoted/resumed/deleted. This intentionally differs from the spool-selection `.tmp` recovery boundary.

### Final source-side classification

```text
DELETE  no new candidates remain from the final owner sweep
MERGE   no duplicate authoritative owners remain from the final owner sweep
KEEP    reviewed live production/build/test/docs/recovery owners
REVIEW  none remain from the named split-owner/crash-residue queue
FIXED   warehouse duplicate Web bootstrap + regression contract
```

Never delete based only on filename or an empty GitHub search result.

### Documentation synchronization

Latest cleanup documentation commits in this pass include:

```text
e52d487c...  06_ACTIVE_WORK_AND_NEXT_STEPS: resolve snapshot crash evidence review
f839dda7...  67_NEXT_CHAT_HANDOFF: close remaining snapshot review
fb51c386...  00_READ_FIRST: sync final cleanup state
3541c268...  AGENTS.md: sync cleanup checkpoint state
fae79b39...  06_ACTIVE_WORK_AND_NEXT_STEPS: record final source cleanup classification
```

The mandatory entrypoints now route to the current handoff instead of stale historical backlog.

## Workflow/source branch state

Production build workflows use `cmp-protocol-v1` as the push source branch, not `main`:

```text
d007a42b...  Arduino Uno Build push branch -> cmp-protocol-v1 only
d0beb03e...  ESP32 Build push branch -> cmp-protocol-v1 only
88273be5...  CI trigger regression forbids returning main
```

The last verified GREEN baseline remains `ad17bb7...` until an applicable later run is actually observed.

## Tree/tests/data status

- `scripts/`, `tools/`, Web shared/reference/sites and root Web selector are reviewed `KEEP`;
- Web contract tests are workflow-owned; no orphan regression test was introduced;
- all 4 host C++ test targets are registered by `Tests/Protocol/CMakeLists.txt`;
- `data/motor_catalog/` is source/reference data -> `KEEP`;
- `CM_JobSpoolSelectionLookup.cpp`, `CM_AutonomousWindingArchiveAssign.cpp`, `CM_WarehouseWriteOffLookup.cpp` remain proven split-owner `KEEP`.

## Remaining cleanup sequence (~1%)

1. obtain an applicable successful CI/Actions result for the latest implementation/test batch;
2. record exact run/SHA as the new verified baseline;
3. mark software cleanup checkpoint complete and synchronize the status from 99% to complete;
4. keep hardware two-board smoke as a separate physical release gate.

## Crash-residue classification

```text
JobStateStore .tmp/.bak        KEEP fail-closed
JobSpoolSelection .json.tmp    KEEP bounded recovery before UART
JobSnapshot .json.tmp          KEEP non-authoritative preparation evidence; no auto-promote/resume/delete
```

Do not unify these merely for symmetry; their durable transaction boundaries differ.

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

Continue directly with code/commits. Do not ask for broad hardware logs during source cleanup; request only the exact Serial interval when an unresolved issue becomes hardware-only.
