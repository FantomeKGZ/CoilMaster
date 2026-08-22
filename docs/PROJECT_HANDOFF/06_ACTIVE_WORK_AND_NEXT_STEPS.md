# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Current USER CONFIRMED GREEN baseline

Пользователь явно подтвердил GREEN для полного текущего provenance/finalization hardening chain через:

```text
e16a7daeae8962e4eb6b457661970f873faf8a87
Align final acceptance exact spool contract
USER CONFIRMED GREEN
```

Это новый применимый GREEN baseline. Более поздние commits нельзя называть GREEN без нового CI/operator подтверждения.

Предыдущие GREEN gates по Arduino Uno, warehouse spool-list cleanup и ранним cleanup chains остаются historical evidence и подробно записаны в старых handoff/checkpoint файлах.

## Full-code audit status

Repo-review/source-contract этапы завершены:

```text
A Arduino safety/realtime/UART/resources                  COMPLETE
B ESP32 runtime/API/persistence/integrity/network/backup COMPLETE
C desktop/mobile/shared Web parity/error/security        COMPLETE
D tests/CI/build-filter/path-trigger audit               COMPLETE
E docs/AI routing consistency                            COMPLETE
```

Authoritative detail: `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`.

## Current active phase — controlled cleanup / zero-debt sweep

Classification policy:

```text
DELETE  proven unused and unreferenced by production/build/tests/docs/runtime
MERGE   duplicate implementation; retain one authoritative owner
KEEP    active production/build/test/docs/history/operator dependency
REVIEW  uncertain dependency; do not delete
```

Rules:

- never delete only because a name contains `Legacy`, `Old`, etc.;
- empty GitHub code-search is not enough proof of no dependency;
- inspect direct owner/call-site/build composition before deletion;
- before modifying an existing file fetch current `cmp-protocol-v1` content and current blob SHA;
- before creating a file verify the exact path does not exist;
- do not weaken safety in the name of cleanup.

## Cleanup completion estimate

**Current controlled cleanup: approximately 92% complete.**

This estimate is deliberately conservative and refers to code/docs/tree cleanup, not complete physical release validation.

Already completed:

- full audit A–E;
- major duplicate/legacy/generated root and docs layers removed;
- obsolete conductor-settings implementation removed;
- obsolete Arduino parallel entrypoint and stale version header removed;
- obsolete Arduino buzzer/start-button implementations removed;
- generated `build/` removed and ignored;
- obsolete warehouse wire catalogue and non-paginated spool-list backend removed;
- obsolete Web calculator helper/injection removed;
- AI routing/docs aligned with cleaned tree;
- active migration/recovery modules separated from real dead code and protected as KEEP;
- ESP32/Arduino owner inventory covered the major production clusters;
- deep UART timeout/recovery review completed;
- exact run/session/spool provenance hardening completed;
- finalization coverage tightened to exact run evidence;
- production KG_FIRST POST/UI/store now require the immutable exact `spool_id`;
- historical `UNALLOCATED` KG_FIRST remains read/recovery compatibility only;
- provenance/finalization chain through `e16a7dae...` is USER CONFIRMED GREEN.

Estimated remaining **~8%**:

1. final owner-by-owner sweep of build-included ESP32/Arduino files for stale/duplicate artifacts;
2. remaining snapshot/state/selection crash-residue consistency REVIEW;
3. final stale wording/test/docs sweep, especially old claims about new-production `UNALLOCATED` write-off;
4. final root/tree zero-debt pass with explicit `DELETE / MERGE / KEEP / REVIEW` classification for any remaining candidates;
5. final documentation consolidation and next-chat handoff maintenance as the tree changes.

Hardware smoke/rescue-restore proof is a separate release-readiness gate and is not counted as software cleanup debt.

## Cleanup completed — important removals

Dependency-proven removals include:

```text
legacy CM_ConductorSettings.* implementation + obsolete source-contract
empty .github placeholder README files
empty LICENSE placeholder
CONTINUE_CMP_PROTOCOL_V1.md
REGISTER.md
CHANGELOG.md
TASKBOOK.md
BUILD_INFO.md
old root ARCHITECTURE.md
capitalized Docs/ legacy documentation
Engineering/ legacy documentation layer
tracked generated build/
old Arduino CM_Buzzer.* / CM_BuzzerController.*
old Arduino CM_StartButton.*
Arduino/CoilMaster_Arduino.ino
Arduino/Config/CM_Version.h
Tests/README.md Build-002A artifact
old untyped CM_WarehouseWireCatalogue.cpp path
firmware/esp32/src/CM_WarehouseSpoolList.cpp
obsolete WarehouseStore::appendActiveSpoolsJson() declaration
firmware/esp32/web/shared/calculator-multisource.js
obsolete StaticSiteServer calculator injection
```

Regression contracts protect the important cleanup boundaries so deleted parallel implementations are not silently reintroduced.

## Confirmed KEEP clusters

Do not treat these as cleanup debt without new direct dependency proof:

```text
CM_WarehouseMaterialCatalogue.cpp
  material-specific CU/AL catalogue for ConductorCalculatorWeb

CM_WarehouseSpoolMaterialList.cpp
  paginated material-aware /api/warehouse/spools backend

CM_WarehouseLegacySpoolMaterial.cpp
  active migration endpoint for old ACTIVE spools missing wire_type

CM_MaterialHistory.cpp + CM_MaterialUsageHistory.cpp
  distinct adjustment and usage journals

CM_JobDisplayRecovery.*
  immutable-snapshot display recovery; no UART/SSR side effects

Arduino/CM_HallCalibrationProtocol.*
Arduino/CM_HardwareControlProtocol.*
Arduino/CM_HallCalibrationService.*
Arduino/CM_HallTelemetry.*
  active Hall/settings/telemetry stack

Arduino/Config/CM_Features.h
Arduino/Config/CM_Pins.h
  authoritative compile-time/hardware configuration

Arduino/Diagnostics/CM_Lcd1602CyrillicTest/CM_Lcd1602CyrillicTest.ino
  standalone diagnostic tool; no production START/SSR/UART ownership

PROJECT.manifest
  current source/boundary manifest

data/motor_catalog/
  active reference catalogue

scripts/ + tools/
  active build-id and motor-reference tooling
```

Important cleanup lesson: `CM_WarehouseLegacySpoolMaterial.cpp` was once wrongly classified unused from empty search results. Direct owner inspection proved the active `POST /api/warehouse/spools/material` path. Search emptiness must never be sole deletion proof.

## Runtime provenance block — completed and GREEN

Authoritative detail: `docs/PROJECT_HANDOFF/64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md`.

Important completed fixes:

```text
late RUN_STARTED after lost JOB_ACK/TIMED_OUT
  narrow exact-session reconciliation only; no auto START/resume

KG_FIRST exact-spool provenance
  current linked production writeoff cannot hide immutable spool_id

linked session persistence integrity
  post-preparation linked state requires matching immutable spool selection

exact-run finalization
  legacy session-only writeoff cannot cover multiple concrete runs

missing selection closure guard
  completed run cannot disappear from coverage because selection evidence is missing

production Web + HTTP alignment
  current KG_FIRST POST/UI uses exact immutable spool only
```

Current migration rule is intentionally asymmetric:

- historical `UNALLOCATED` KG_FIRST records remain readable/auditable/recoverable;
- new linked production mutations are exact-spool only;
- a future true unallocated production workflow, if ever required, must create an immutable material selection **before** the UART boundary, not after `RUN_COMPLETED`.

## Remaining crash-residue REVIEW

`JobStateStore` intentionally fails closed when atomic replacement `.tmp/.bak` residue exists. Existing contract protects `target -> backup -> verified candidate -> target -> verify -> cleanup` ordering and forbids deleting the only authoritative state as a shortcut.

`JobSnapshotStore` orphan `.json.tmp` remains **REVIEW / resilience**, not a current safety defect. Snapshot creation occurs before CREATED state and before UART delivery, so orphan temp evidence cannot itself start/resume hardware. Do not introduce ad-hoc automatic deletion; any recovery policy must be consistent across snapshot/state/selection stores.

Direct append tail resilience for production NDJSON is also not a cleanup deletion task. Never auto-truncate malformed/torn production evidence.

## Current next cleanup sequence

1. continue final ESP32/Arduino owner inventory using build/call-site proof;
2. inspect suspicious stale/duplicate names but classify before changing anything;
3. complete snapshot/state/selection crash-residue consistency review;
4. sweep tests/docs for stale pre-fix contracts, especially new-production unallocated wording;
5. final root/tree/docs reference zero-debt pass;
6. keep `docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md` synchronized for transition;
7. ask for exact Serial/runtime capture only when the remaining unresolved item actually requires hardware proof.

## External hardware verification gate

Targeted ESP32<->Arduino smoke remains required for final runtime release confidence when the stand is available, but it does not block deletion of dependency-proven dead software:

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
- manual writeoff uses exact `source_session_id + source_run_id`;
- current linked production writeoff retains exact immutable `spool_id`;
- historical `UNALLOCATED` records are compatibility evidence, not permission to drop a selected spool from a new run;
- backup/restore is explicit, operator-only, transactional/fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion.

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

New chat should continue directly from the current branch state. Do not restart the full audit unless a concrete inconsistency requires it.
