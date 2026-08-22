# CoilMaster — AI agent entrypoint

This file is the mandatory starting point for any AI/coding agent changing CoilMaster.

## Source of truth

- Repository: `FantomeKGZ/CoilMaster`
- Working/source-of-truth branch: **`cmp-protocol-v1`**
- Do **not** use `main` as implementation source.
- Before editing an existing file, fetch its current contents from `cmp-protocol-v1` and use the current blob SHA.
- Before creating a new file, verify that the exact path does not exist.
- Do not claim build/CI/hardware status unless it was actually confirmed.

## Mandatory read order

1. `docs/PROJECT_HANDOFF/00_READ_FIRST.md`
2. `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`
3. `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`
4. `docs/AI_AGENT/00_START_HERE.md`
5. `docs/AI_AGENT/01_PROJECT_MAP.md`
6. `docs/AI_AGENT/02_CHANGE_ROUTER.md`
7. `docs/AI_AGENT/03_ADD_MODULE_PLAYBOOK.md` when adding a module/service/page/storage domain
8. `docs/AI_AGENT/04_VERIFICATION_MATRIX.md`
9. Only then open the exact production files needed for the task.

Older numbered handoff checkpoints are historical evidence, not an active task queue. Do not resume work merely because an old checkpoint says `next`.

## Current active phase

The full repo-level code audit A–E is complete. Active repo-level work is now:

```text
CONTROLLED REPOSITORY CLEANUP / DE-DUPLICATION
```

Every cleanup candidate must be classified before deletion:

```text
DELETE  proven unused and unreferenced by production/build/tests/docs/runtime
MERGE   duplicate implementation; retain one authoritative owner
KEEP    active production/build/test/docs/history/operator dependency
REVIEW  uncertain dependency; do not delete
```

Do not mix speculative redesign with cleanup. Do not delete production data or historical evidence automatically.

## Current verified repo baseline

Latest user-confirmed GREEN baseline before the next cleanup batch:

```text
51ea46c1823a451e7f80ecd188daf896aafc752d
Fix production conductor cleanup contract
USER CONFIRMED GREEN
```

Commits after this SHA require a fresh exact result or explicit user confirmation before being described as GREEN.

## Non-negotiable safety invariants

Never weaken these contracts:

- physical START is local/physical only;
- no automatic physical START between repeats;
- ESP32/Web never control SSR directly;
- no automatic winding resume after reboot;
- lost ACK / timeout never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never performs automatic wire writeoff;
- manual material writeoff requires exact `source_session_id + source_run_id` provenance;
- `spool_id` is optional only in the approved KG-first unallocated/manual path;
- if a spool is used, exact spool provenance must be preserved;
- backup restore is operator-only, transactional and fail-closed;
- reboot never auto-continues restore/apply;
- persisted restore evidence blocks unsafe backup/restore operations until explicitly resolved;
- microSD pressure never triggers automatic deletion of production data;
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
- update `00_READ_FIRST.md` and the current active queue when project status materially changes;
- keep older numbered checkpoints historical; do not rewrite them into an active queue.

During cleanup, update documentation that points to deleted files or stale owners in the same cleanup batch.

The goal is that the next agent starts from the current code/status without repository-wide archaeology or stale chat history.
