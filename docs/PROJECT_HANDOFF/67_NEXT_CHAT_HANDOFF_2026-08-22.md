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

## Closed CI incident

The prior RED series was stale regression-test debt, not production runtime failure. Two corrections closed it:

```text
9fc671121f86b5b25f06e5c59adcd8a9e3d7f154
  release test updated from optional spool to mandatory exact spool

ad17bb7029f9f0f694fcb275ce729d0c23c8e1dd
  obsolete automatic_* JSON string checks replaced with semantic safety checks
```

Do not reopen optional-spool or presentation-string behavior to satisfy old tests.

## Cleanup progress

Controlled code/docs/tree cleanup: **~96% complete**. Remaining: **~4%**.

Hardware E2E is a separate release gate and is not included in this cleanup percentage.

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

## Remaining cleanup sequence (~4%)

1. continue final owner-by-owner ESP32/Arduino/Core sweep;
2. finish thematic stale test/doc sweep;
3. final root/tree/Web/shared/scripts/tools zero-debt pass;
4. classify all remaining candidates `DELETE / MERGE / KEEP / REVIEW`;
5. synchronize this file with `06_ACTIVE_WORK_AND_NEXT_STEPS.md` and create final cleanup checkpoint.

## Already completed major cleanup

- full audit A–E;
- obsolete Arduino parallel `.ino`, buzzer/start-button code and `CM_Version.h` removed;
- old conductor settings implementation removed;
- generated `build/` removed/ignored;
- obsolete warehouse wire catalogue and non-paginated spool list removed;
- obsolete calculator helper/injection removed;
- Arduino/Core state-machine defects fixed and regression-tested;
- late `RUN_STARTED` after timeout recovery fixed;
- exact run/session/spool provenance hardened;
- finalization requires exact-run writeoff coverage and immutable selection evidence;
- KG_FIRST current POST/store/UI requires exact immutable spool;
- historical unallocated records remain compatible without authorizing new optional-spool writes;
- crash-residue policies reviewed by transaction boundary;
- top-level AI/handoff routing aligned to current exact-spool model;
- release safety regression contract corrected and CI-verified GREEN.

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
