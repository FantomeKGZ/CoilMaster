# Next chat handoff — CoilMaster cleanup — 2026-08-23

## Source of truth

```text
Repository: FantomeKGZ/CoilMaster
Branch: cmp-protocol-v1
main: DO NOT use as source code
```

Always fetch current file content from `cmp-protocol-v1` and use the current blob SHA before updating/deleting an existing file. Verify an exact path is absent before creating a new file.

## Current verified baseline

The user explicitly confirmed the provenance/finalization hardening chain GREEN through:

```text
e16a7daeae8962e4eb6b457661970f873faf8a87
Align final acceptance exact spool contract
USER CONFIRMED GREEN
```

Later commits include documentation cleanup and one regression-test correction. Do not call those later commits CI GREEN until fresh Actions evidence is available.

## Current CI issue/fix

The user supplied 17 failed Actions runs:

```text
32585065786 32585109948 32585215490 32585252831 32585283088
32585353529 32588852728 32588893427 32588921793 32589153377
32589190184 32589218091 32589251728 32589275453 32589344988
32589369003 32589399093
```

They are not evidence of 17 separate runtime defects. Inspected jobs/logs show the persistent failure is the same stale test:

```text
Audit release safety contracts
Tests/Web/check_release_contracts.js
```

The test still expected the previous optional-spool expression:

```text
selection.repairId != repairId || (spoolId != 0UL && selection.spoolId != spoolId)
```

while current production correctly requires exact immutable spool identity:

```text
selection.repairId != repairId || selection.spoolId != spoolId
```

The earliest inspected run `32585065786` also contained stale KG_FIRST assertions for unallocated/spool-optional behavior. Later inspected runs show `Audit kg-first material contracts` GREEN, so that older mismatch was already resolved.

Regression fix committed:

```text
9fc671121f86b5b25f06e5c59adcd8a9e3d7f154
Align release safety contract with exact spool provenance
```

The release test now explicitly requires `spool_id_required_for_kg_first` plus mandatory exact selection-spool equality.

**Next action:** inspect fresh Actions on/after `9fc6711...`. If red, fetch exact first failed step/log; do not roll production back to the old optional-spool behavior.

## Current cleanup progress

Estimated controlled code/docs/tree cleanup completion: **~95%**.

Estimated cleanup remaining: **~5%**.

This is a cleanup estimate, not total product release readiness. Physical hardware smoke/rescue-restore proof is a separate gate.

### What is already complete

- full audit A–E: Arduino, ESP32, Web, tests/CI, docs/AI routing;
- major legacy/generated/duplicate tree cleanup;
- obsolete Arduino parallel entrypoint, buzzer/start-button code and stale version header cleanup;
- obsolete conductor settings implementation cleanup;
- obsolete warehouse wire catalogue and old non-paginated spool-list cleanup;
- obsolete Web calculator helper/injection cleanup;
- root/docs routing cleanup;
- direct-owner KEEP classification for active migration/recovery/diagnostic modules;
- UART lost-ACK / timeout / late RUN_STARTED review;
- immutable job/session/spool provenance review;
- manual exact-run writeoff and crash-recovery review;
- exact-run finalization coverage hardening;
- linked spool-selection persistence/closure hardening;
- current KG_FIRST store + HTTP + desktop/mobile UI aligned to exact immutable spool;
- historical unallocated writeoff compatibility retained read-only/recovery-side;
- snapshot/state/selection crash-residue policy reviewed and classified by transaction boundary;
- top-level/AI maintenance routing aligned with exact-spool model;
- repeated stale release-safety regression identified and corrected at `9fc6711...`.

Authoritative runtime/provenance detail:

```text
docs/PROJECT_HANDOFF/64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md
```

## Current production flow

```text
client
-> motor
-> OPEN repair
-> costing
-> linked winding
-> exact immutable spool selection
-> immutable job snapshot/state
-> UART JOB
-> physical START
-> RUN_STARTED / RUN_COMPLETED
-> explicit manual exact-run exact-spool wire writeoff
-> costing / finalization preflight
-> CLOSED
-> reports
-> backup
```

## Critical material provenance rule

For current linked production:

```text
source_session_id + source_run_id + exact immutable spool_id
```

must remain tied together for manual wire writeoff.

Historical `UNALLOCATED` KG_FIRST records remain readable/auditable/recoverable because old evidence must not be destroyed or rewritten. They are **not** permission for a new linked production writeoff to omit an already selected spool.

If a true unallocated production workflow is ever required, it must create immutable material provenance before the UART boundary. Do not add a post-run fallback that drops `spool_id` after `RUN_COMPLETED`.

## Crash-residue classification — reviewed

Do not reopen this as generic "make all stores identical" cleanup.

```text
JobStateStore .tmp/.bak
  KEEP fail-closed
  replacement may involve an older authoritative runtime state, so residue is ambiguous

JobSpoolSelectionStore .json.tmp
  KEEP bounded recovery
  selection is written after durable CREATED state but before DELIVERING/UART;
  begin() accepts only one fully valid temp, requires final to be absent and only renames temp -> final

JobSnapshotStore .json.tmp
  REVIEW / fail-closed resilience
  snapshot temp exists before durable state; do not auto-delete or auto-promote it ad hoc
```

The policies differ because the files represent different transaction boundaries, not because cleanup is incomplete.

## Safety invariants — do not weaken

- no automatic physical START;
- no automatic START between repeat runs;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout alone never proves Arduino idle;
- final repeat cannot automatically reopen;
- `RUN_COMPLETED` never automatically deducts material;
- writeoff remains explicit/manual and exact-run/exact-spool for current linked production;
- cancellation must not erase immutable run/history evidence;
- restore is explicit/operator-only and fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion or automatic NDJSON truncation.

## Remaining cleanup — continue directly (~5%)

### 1. Verify fresh post-fix CI

Check fresh Actions on/after `9fc6711...`. Record exact workflow/run result. If failure remains, inspect first failed step/log before editing.

### 2. Final ESP32/Arduino owner inventory

Continue owner-by-owner over build-included source. For each suspicious duplicate/stale file classify:

```text
DELETE / MERGE / KEEP / REVIEW
```

Do not use empty GitHub code-search as sole deletion proof. Inspect build ownership, direct call-sites, API route ownership, persistence/recovery dependencies, tests, and operator workflow.

### 3. Remaining stale contracts/docs sweep

Continue checking thematic docs/tests for statements that imply **new production** KG_FIRST may omit `spool_id` or choose a post-run `UNALLOCATED` fallback. Historical schema/recovery references to old unallocated records are valid and should remain.

### 4. Final zero-debt tree pass

Check top-level tree, production source directories, tests, scripts/tools, docs references and Web shared assets. Preserve known KEEP clusters unless new direct proof shows otherwise.

### 5. Final handoff consolidation

Keep this file and `06_ACTIVE_WORK_AND_NEXT_STEPS.md` synchronized. Do not restart completed audit/provenance/residue review in the next chat unless current source gives a concrete inconsistency.

### 6. Hardware/runtime logs only when needed

Do not ask the user for broad logs during source cleanup. When a remaining issue is hardware-only, request the exact Serial interval needed.

## Known KEEP clusters

```text
CM_WarehouseMaterialCatalogue.cpp
CM_WarehouseSpoolMaterialList.cpp
CM_WarehouseLegacySpoolMaterial.cpp
CM_MaterialHistory.cpp
CM_MaterialUsageHistory.cpp
CM_JobDisplayRecovery.*
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
```

Important historical lesson: `CM_WarehouseLegacySpoolMaterial.cpp` was once wrongly treated as unused because search indexing returned no call-site. Direct owner inspection proved that it owns active legacy spool material migration. Never repeat that deletion method.

## Main removed artifacts

Already removed and generally protected by tests/contracts:

```text
old CM_ConductorSettings.* implementation
old Arduino CM_Buzzer.* / CM_BuzzerController.*
old Arduino CM_StartButton.*
Arduino/CoilMaster_Arduino.ino
Arduino/Config/CM_Version.h
CM_WarehouseWireCatalogue.cpp old untyped path
CM_WarehouseSpoolList.cpp old non-paginated path
WarehouseStore::appendActiveSpoolsJson() obsolete API
calculator-multisource.js + obsolete injection
tracked build/
Engineering/
capitalized Docs/
old root ARCHITECTURE.md
obsolete root checkpoint/placeholder docs
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

## Working style requested by user

Continue directly with code/commits rather than long planning. Keep handoff docs current as changes land. Do not ask the user to manually verify every commit. Do not claim CI/build GREEN without direct evidence or explicit user confirmation.
