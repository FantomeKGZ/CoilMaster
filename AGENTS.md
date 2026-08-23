# CoilMaster — AI agent entrypoint

This file is the mandatory starting point for any AI/coding agent changing CoilMaster.

## Source of truth

- Repository: `FantomeKGZ/CoilMaster`
- Working/source-of-truth branch: **`cmp-protocol-v1`**
- Do **not** use `main` as implementation source.
- Before editing/deleting an existing file, fetch its current contents from `cmp-protocol-v1` and use the current blob SHA.
- Before creating a new file, verify that the exact path does not exist.
- Do not claim build/CI/hardware status unless it was actually confirmed.

## Mandatory read order

1. `docs/PROJECT_HANDOFF/00_READ_FIRST.md`
2. `docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md`
3. `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`
4. `docs/PROJECT_HANDOFF/64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md`
5. `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`
6. `docs/AI_AGENT/00_START_HERE.md`
7. `docs/AI_AGENT/01_PROJECT_MAP.md`
8. `docs/AI_AGENT/02_CHANGE_ROUTER.md`
9. `docs/AI_AGENT/03_ADD_MODULE_PLAYBOOK.md` when adding a module/service/page/storage domain
10. `docs/AI_AGENT/04_VERIFICATION_MATRIX.md`
11. Only then open the exact production files needed for the task.

Older numbered handoff checkpoints are historical evidence, not an active task queue. Do not resume work merely because an old checkpoint says `next`.

## Current phase

The full repo-level code audit A–E is complete.

The final controlled repository cleanup / zero-debt sweep is also complete:

```text
SOFTWARE CLEANUP COMPLETE — 100%
```

The latest cleanup implementation/test batch was confirmed GREEN by the operator on **2026-08-23**. The final implementation/test cleanup commit is:

```text
bd64e3cc4ba92a6624aed677d98c1620c165013e
test(warehouse): guard against duplicate web bootstrap
```

It protects:

```text
06a752663504d58ca6908414f8aa8786007c6877
fix(esp32): remove duplicate warehouse web bootstrap
```

Do not restart broad cleanup without a concrete new inconsistency, failing test, runtime defect, or stale-contract evidence.

## Final cleanup classification

```text
DELETE  no remaining proven cleanup candidates
MERGE   no duplicate authoritative owners remain
KEEP    reviewed live production/build/test/docs/recovery owners
REVIEW  none remain in the named cleanup queue
FIXED   warehouse duplicate Web bootstrap + regression contract
CI      current Actions confirmed GREEN by operator on 2026-08-23
```

## Non-negotiable safety invariants

Never weaken these contracts:

- physical START is local/physical only;
- no automatic physical START between repeats;
- ESP32/Web never control SSR directly;
- no automatic winding resume after reboot;
- lost ACK / timeout never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never performs automatic wire writeoff;
- current linked-production manual material writeoff requires exact `source_session_id + source_run_id + immutable spool_id` provenance;
- historical `UNALLOCATED` KG_FIRST records remain read/audit/recovery compatibility evidence only and do not authorize a new linked writeoff to omit an already selected spool;
- backup restore is operator-only, transactional and fail-closed;
- reboot never auto-continues restore/apply;
- persisted restore evidence blocks unsafe backup/restore operations until explicitly resolved;
- microSD pressure never triggers automatic deletion of production data;
- malformed/torn production evidence is never automatically truncated as a cleanup shortcut;
- destructive fault injection is forbidden on the working production microSD.

A change touching one of these boundaries requires targeted regression verification.

## Production source boundaries

`platformio.ini` defines the authoritative build inputs.

Arduino Uno production build:

```text
Core/*.cpp
Arduino/*.cpp
firmware/arduino/src/main.cpp
```

ESP32 production build:

```text
firmware/esp32/src/*.cpp
```

Web assets deployed to microSD:

```text
firmware/esp32/web/
```

Do not assume a similarly named or differently capitalized directory is production code.

## Architectural split

### Arduino Uno owns realtime machine safety

- physical START
- SSR
- Hall turns
- keypad/LCD/buzzer
- local winding state machine
- execution of an accepted remote job
- RUN_STARTED/RUN_COMPLETED events

### ESP32 owns service/data/UI orchestration

- Wi-Fi/AP/mDNS/HTTP/FTP
- microSD and RTC
- clients/motors/repairs
- winding job preparation and persistence
- warehouse/materials/costing
- event journal
- backup/restore
- web UI and diagnostics

Main integration points:

```text
firmware/arduino/src/main.cpp
firmware/esp32/src/main.cpp
firmware/esp32/src/CM_StaticSiteServer.h/.cpp
```

## Protocol boundary

Production UART is text `CMP1|...`:

```text
Arduino/CM_UartEventTransport.h/.cpp
firmware/esp32/src/CM_UartEventReceiver.h/.cpp
Shared/CMP1Text/CM_Cmp1Crc.h
```

`Shared/Protocol/` is the older binary CMP host-test protocol and is **not** a drop-in replacement for production CMP1.

Any production UART frame change must be reviewed on both boards, in tests, compatibility rules and documentation.

## Persistence transaction boundary note

Do not force all winding-job temp files into one generic recovery policy:

```text
JobStateStore .tmp/.bak
  KEEP fail-closed replacement evidence

JobSpoolSelectionStore .json.tmp
  KEEP bounded recovery of one fully valid pre-UART selection temp when final is absent

JobSnapshotStore .json.tmp
  KEEP non-authoritative preparation crash evidence; no auto-promote/resume/delete
```

The different policies reflect different durable transaction boundaries.

## Fast change rule

Before coding, identify all five parts of the change:

```text
OWNER -> CONTRACT -> PERSISTENCE -> UI/API -> VERIFICATION
```

If one is not applicable, state why. Do not add a hidden subsystem with no clear owner or lifecycle.

## Documentation rule

When a production capability is added, moved or materially changes state:

- update the relevant `docs/AI_AGENT/` map/router entry;
- update the thematic project doc when a public/data/protocol contract changes;
- keep `docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md` and `06_ACTIVE_WORK_AND_NEXT_STEPS.md` synchronized when status materially changes;
- update `00_READ_FIRST.md` if the mandatory starting route or high-level state changes;
- keep older numbered checkpoints historical; do not rewrite them into an active queue.

## External hardware gate

Software cleanup completion does not imply hardware release completion. Targeted two-board ESP32<->Arduino UART/repeat/cancel/reboot smoke remains the physical release gate when needed. Hardware GREEN is never inferred from CI.

## Next-work rule

Software cleanup is closed. Continue from a concrete product/runtime goal, hardware verification result, bug, feature, or documentation contract change. Do not reopen completed cleanup/A–E/provenance/crash-residue audits without new evidence.
