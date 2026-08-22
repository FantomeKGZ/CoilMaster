# Next chat handoff — CoilMaster cleanup — 2026-08-22

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

Documentation commits after that confirmation are not independent firmware GREEN evidence and must not be described as such.

## Current cleanup progress

Estimated controlled code/docs/tree cleanup completion: **~92%**.

Estimated cleanup remaining: **~8%**.

This is a cleanup estimate, not total product release readiness. Physical hardware smoke/rescue-restore proof is a separate gate.

### What is already complete

- full audit A–E: Arduino, ESP32, Web, tests/CI, docs/AI routing;
- major legacy/generated/duplicate tree cleanup;
- obsolete Arduino parallel entrypoint cleanup;
- obsolete Arduino buzzer/start-button implementation cleanup;
- stale Arduino version header cleanup;
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
- top-level final acceptance contract aligned with current exact-spool production rule.

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
-> explicit manual exact-run wire writeoff
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

## Safety invariants — do not weaken

- no automatic physical START;
- no automatic START between repeat runs;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout alone never proves Arduino idle;
- final repeat cannot automatically reopen;
- `RUN_COMPLETED` never automatically deducts material;
- writeoff remains explicit/manual and exact-run;
- cancellation must not erase immutable run/history evidence;
- restore is explicit/operator-only and fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion or automatic NDJSON truncation.

## Remaining cleanup — continue directly

### 1. Final ESP32/Arduino owner inventory

Continue owner-by-owner over build-included source. For each suspicious duplicate/stale file classify:

```text
DELETE / MERGE / KEEP / REVIEW
```

Do not use empty GitHub code-search as sole deletion proof. Inspect build ownership, direct call-sites, API route ownership, persistence/recovery dependencies, tests, and operator workflow.

### 2. Snapshot/state/selection crash residue consistency

Current known state:

- `JobStateStore` intentionally fails closed on `.tmp/.bak` residue;
- its contract protects atomic rotate/commit/verify/cleanup ordering;
- `JobSnapshotStore` orphan `.json.tmp` is currently REVIEW/resilience, not a proven safety defect because snapshot creation is before CREATED state/UART;
- do not introduce ad-hoc automatic deletion;
- compare snapshot/state/selection policy coherently before changing recovery behavior.

### 3. Stale contract/docs sweep

Search for stale statements that imply **new production** KG_FIRST may omit `spool_id` or choose post-run `UNALLOCATED` fallback. Historical schema/recovery references to old unallocated records are valid and should remain.

### 4. Final zero-debt tree pass

Check top-level tree, production source directories, tests, scripts/tools, docs references, and Web shared assets. Preserve known KEEP clusters unless new direct proof shows otherwise.

### 5. Hardware/runtime logs only when needed

Do not ask the user for broad logs during source cleanup. When a remaining issue is hardware-only, request the exact Serial interval needed, for example from before JOB send through ACK/READY, physical START and RUN_STARTED/RUN_COMPLETED, or the specific cancel/reboot sequence under review.

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
