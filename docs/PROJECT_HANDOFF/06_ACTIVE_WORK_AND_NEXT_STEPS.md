# Активная работа и следующие шаги

Дата обновления: **2026-08-23**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Current USER CONFIRMED GREEN baseline

Пользователь явно подтвердил GREEN для полного provenance/finalization hardening chain через:

```text
e16a7daeae8962e4eb6b457661970f873faf8a87
Align final acceptance exact spool contract
USER CONFIRMED GREEN
```

Это применимый implementation GREEN baseline. Более поздние documentation commits не являются новым firmware GREEN доказательством. Более поздние implementation/test changes нельзя называть GREEN без нового CI/operator подтверждения.

## CI incident after exact-spool hardening — diagnosed

Пользователь передал 17 failed GitHub Actions runs:

```text
32585065786 32585109948 32585215490 32585252831 32585283088
32585353529 32588852728 32588893427 32588921793 32589153377
32589190184 32589218091 32589251728 32589275453 32589344988
32589369003 32589399093
```

Observed failure pattern from workflow jobs/logs:

- CMake configure/build and all 4 host C++ tests are GREEN in inspected runs;
- the persistent failure is `Audit release safety contracts` -> `Tests/Web/check_release_contracts.js`;
- failing assertion expected the old optional-spool condition:
  `selection.repairId != repairId || (spoolId != 0UL && selection.spoolId != spoolId)`;
- current production code correctly uses mandatory exact-spool matching:
  `selection.repairId != repairId || selection.spoolId != spoolId`;
- earliest inspected run `32585065786` also had an old `Audit kg-first material contracts` failure for unallocated/spool-optional guards; later inspected runs show that KG_FIRST contract already GREEN, leaving only the stale release-safety assertion.

Fix landed:

```text
9fc671121f86b5b25f06e5c59adcd8a9e3d7f154
Align release safety contract with exact spool provenance
```

The release regression now explicitly requires:

```text
spool_id_required_for_kg_first
selection.repairId != repairId || selection.spoolId != spoolId
```

and describes historical `UNALLOCATED` only as compatibility evidence, not a current production bypass.

Status: **fix committed; fresh GitHub Actions result still required before calling this post-fix HEAD CI GREEN**.

## Full-code audit status

```text
A Arduino safety/realtime/UART/resources                  COMPLETE
B ESP32 runtime/API/persistence/integrity/network/backup COMPLETE
C desktop/mobile/shared Web parity/error/security        COMPLETE
D tests/CI/build-filter/path-trigger audit               COMPLETE
E docs/AI routing consistency                            COMPLETE
```

Detail: `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`.

## Current active phase — final controlled cleanup / zero-debt sweep

Classification:

```text
DELETE  proven unused and unreferenced
MERGE   proven duplicate ownership
KEEP    active production/build/test/docs/history/operator dependency
REVIEW  uncertain or intentionally unresolved dependency/policy
```

Never delete from filename alone. Empty GitHub code search is supporting evidence only, never proof. Fetch current file + SHA before every edit/delete.

## Cleanup completion estimate

**Current controlled cleanup: approximately 95% complete.**

**Estimated remaining cleanup: approximately 5%.**

This estimate is for software code/docs/tree cleanup. Physical hardware smoke/rescue-restore confidence is a separate release gate.

### Completed cleanup/audit blocks

- full audit A–E;
- major duplicate/legacy/generated root/docs cleanup;
- obsolete conductor-settings implementation removed;
- obsolete Arduino parallel entrypoint, stale version header, buzzer/start-button implementations removed;
- generated `build/` removed/ignored;
- obsolete warehouse wire catalogue and non-paginated spool-list API/backend removed;
- obsolete Web calculator helper/injection removed;
- active migration/recovery modules separated from dead code and protected as KEEP;
- major ESP32/Arduino production owner inventory completed;
- UART lost-ACK/timeout/late-RUN_STARTED review completed;
- exact run/session/spool provenance hardening completed;
- exact-run finalization coverage completed;
- missing immutable selection now blocks deep integrity and closure;
- current KG_FIRST store/API/desktop/mobile UI require exact immutable `spool_id`;
- historical `UNALLOCATED` KG_FIRST remains read/audit/recovery compatibility only;
- runtime provenance/finalization chain through `e16a7dae...` USER CONFIRMED GREEN;
- snapshot/state/selection crash-residue policy reviewed/classified by transaction boundary;
- AI/top-level routing aligned to current GREEN/exact-spool model;
- stale release-safety regression found from repeated red Actions and fixed at `9fc6711...`.

## Remaining ~5%

1. obtain fresh post-`9fc6711...` Actions evidence and record exact result;
2. final owner-by-owner sweep of build-included ESP32/Arduino files for stale/duplicate artifacts;
3. remaining thematic docs/tests stale-contract sweep outside already corrected routing/release contracts;
4. final top-level/tree/Web/shared/scripts/tools zero-debt pass;
5. explicit final classification of any remaining candidates as `DELETE / MERGE / KEEP / REVIEW`;
6. final handoff consolidation for the next chat.

## Important completed removals

```text
legacy CM_ConductorSettings.* implementation
empty .github placeholder README files
empty LICENSE placeholder
CONTINUE_CMP_PROTOCOL_V1.md
REGISTER.md
CHANGELOG.md
TASKBOOK.md
BUILD_INFO.md
old root ARCHITECTURE.md
capitalized Docs/
Engineering/
tracked generated build/
old Arduino CM_Buzzer.* / CM_BuzzerController.*
old Arduino CM_StartButton.*
Arduino/CoilMaster_Arduino.ino
Arduino/Config/CM_Version.h
Tests/README.md
old untyped CM_WarehouseWireCatalogue.cpp
firmware/esp32/src/CM_WarehouseSpoolList.cpp
WarehouseStore::appendActiveSpoolsJson() obsolete declaration
firmware/esp32/web/shared/calculator-multisource.js
obsolete StaticSiteServer calculator injection
```

## Confirmed KEEP clusters

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

Important lesson: `CM_WarehouseLegacySpoolMaterial.cpp` was once wrongly classified unused from empty code-search. Direct route/owner inspection proved active `POST /api/warehouse/spools/material`. Never use search emptiness as sole deletion proof.

## Runtime provenance block — completed and GREEN

Detail: `docs/PROJECT_HANDOFF/64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md`.

Current production material rule:

```text
source_session_id + source_run_id + exact immutable spool_id
```

for current linked-production manual writeoff.

Historical `UNALLOCATED` records are immutable compatibility evidence only. A future true unallocated production mode would require immutable material provenance before UART; never recreate it as a post-`RUN_COMPLETED` fallback.

## Crash-residue consistency — REVIEW completed

Current classification is intentional:

```text
JobStateStore .tmp/.bak
  KEEP fail-closed
  atomic replacement may have old/new authoritative runtime state ambiguity

JobSpoolSelectionStore .json.tmp
  KEEP bounded recovery
  selection is after durable CREATED state but before DELIVERING/UART;
  only one fully valid temp with absent final is renamed temp -> final

JobSnapshotStore .json.tmp
  REVIEW / fail-closed resilience
  snapshot is before durable state; do not auto-delete/promote ad hoc
```

Do not force these into one policy merely for symmetry. Their transaction boundaries differ. Production NDJSON malformed/torn evidence also must never be auto-truncated.

## Current next cleanup sequence

1. verify fresh CI after `9fc6711...`; if red, inspect exact first failed step/log before changing source;
2. continue final ESP32/Arduino owner inventory with direct build/call-site proof;
3. inspect suspicious stale/duplicate names and classify before changing;
4. sweep remaining thematic tests/docs for pre-fix exact-spool contradictions;
5. final root/tree/Web/shared/scripts/tools reference pass;
6. keep `67_NEXT_CHAT_HANDOFF_2026-08-22.md` synchronized;
7. request exact Serial/runtime capture only when an unresolved item truly becomes hardware-only.

## External hardware verification gate

Targeted ESP32<->Arduino smoke remains separate from cleanup completion:

```text
normal JOB -> Arduino READY
physical START only
RUN_STARTED -> RUN_COMPLETED
repeat > 1 -> physical START each run
no automatic material writeoff
zero-run cancel / ALREADY_CLEAR / safe ALL_CLEAR
late zero-id ALL_CLEAR must not cancel fresh job
lost JOB_ACK -> timeout/manual review -> late RUN_STARTED reconciliation
reboot waiting/running -> no auto resume
```

Hardware GREEN is never inferred from CI.

## Safety boundary — never change during cleanup

- physical START is physical/local only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web do not drive SSR directly;
- timeout/lost ACK alone never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never auto-writes off material;
- current linked manual writeoff is exact session + run + immutable spool;
- historical unallocated records do not authorize dropping a selected spool;
- backup/restore explicit/operator-only/transactional/fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion/truncation.

## Read order for a new chat / AI

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

New chat should continue directly from the current branch state. Do not restart completed audit/provenance/crash-residue review unless current source proves a concrete inconsistency.
