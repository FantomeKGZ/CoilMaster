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

В run `32616937088` прошли CMake configure/build, все 4 host C++ tests и весь Web/Protocol contract suite, включая ранее падавший `Audit release safety contracts`.

## Закрытый CI incident — release safety regression

Предыдущая серия RED была вызвана stale `Tests/Web/check_release_contracts.js`, а не production runtime. Последовательно были закрыты два устаревших ожидания:

```text
9fc671121f86b5b25f06e5c59adcd8a9e3d7f154
  optional-spool assertion -> mandatory exact-spool assertion

ad17bb7029f9f0f694fcb275ce729d0c23c8e1dd
  presentation-only automatic_* JSON assertions -> semantic safety guards
```

Release contract теперь проверяет фактическую модель: no auto queue/resume on recovery, timeout -> manual review, absence of forbidden auto-start/auto-resume/auto-writeoff calls, exact immutable spool + exact session/run manual writeoff.

## Cleanup status

Full audit A–E завершён. Controlled cleanup оценивается примерно в **96%**, осталось около **4%**. Hardware smoke/recovery verification — отдельный release gate и в эти 4% не входит.

### Уже завершено

- major duplicate/legacy/generated root/docs cleanup;
- obsolete Arduino parallel entrypoint, buzzer/start-button code, stale version header removed;
- obsolete conductor settings implementation removed;
- generated `build/` removed/ignored;
- old warehouse wire catalogue and non-paginated spool list removed;
- obsolete Web calculator helper/injection removed;
- active migration/recovery modules classified/protected as KEEP;
- Arduino/Core state-machine audit and regressions;
- UART lost-ACK/timeout/late-RUN_STARTED review;
- exact run/session/spool provenance hardening;
- exact-run finalization coverage;
- immutable selection required by deep integrity and closure;
- KG_FIRST current store/API/UI requires exact immutable `spool_id`;
- historical `UNALLOCATED` remains read/audit/recovery compatibility only;
- snapshot/state/selection crash-residue policy reviewed by transaction boundary;
- top-level and AI routing aligned with current exact-spool model;
- stale release-safety regression corrected and CI-verified GREEN at `ad17bb7...`.

## Remaining ~4%

1. final owner-by-owner sweep of build-included ESP32/Arduino/Core files;
2. remaining thematic stale-contract/docs sweep;
3. final root/tree/Web/shared/scripts/tools zero-debt pass;
4. explicit final classification of remaining candidates as `DELETE / MERGE / KEEP / REVIEW`;
5. final handoff consolidation and cleanup-complete checkpoint.

## Current production material rule

For new linked production manual writeoff:

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
  REVIEW / fail-closed resilience
```

Do not force these stores into one recovery policy; their transaction boundaries differ.

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

Important lesson: empty GitHub code-search is never sufficient deletion proof. `CM_WarehouseLegacySpoolMaterial.cpp` was once wrongly classified from an empty search and had to be restored after direct route-owner inspection.

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
