# CoilMaster — AI agent entrypoint

This file is the mandatory starting point for any AI/coding agent changing CoilMaster.

## Source of truth

- Repository: `FantomeKGZ/CoilMaster`
- Working/source-of-truth branch: **`cmp-protocol-v1`**
- Do **not** use `main` as implementation source.
- Before editing an existing file, fetch its current contents from `cmp-protocol-v1` and use the current blob SHA.
- Before creating a new file, verify that the exact path does not exist.
- Do not claim build/CI/hardware status unless it was actually confirmed.

## Read order before a change

1. `docs/AI_AGENT/00_START_HERE.md`
2. `docs/AI_AGENT/01_PROJECT_MAP.md`
3. `docs/AI_AGENT/02_CHANGE_ROUTER.md`
4. `docs/AI_AGENT/03_ADD_MODULE_PLAYBOOK.md` when adding a module/service/page/storage domain
5. `docs/AI_AGENT/04_VERIFICATION_MATRIX.md`
6. Current release checkpoint: `docs/PROJECT_HANDOFF/38_COILMASTER_V1_RELEASE_READY_2026-08-16.md`
7. Only then open the exact production files listed by the router.

Historical handoff files are context, not a replacement for current code.

## Non-negotiable safety invariants

Never weaken these contracts:

- physical START is local/physical only;
- ESP32/Web never control SSR directly;
- no automatic winding resume after reboot;
- `RUN_COMPLETED` never performs automatic wire writeoff;
- wire writeoff remains manual and exact `spool_id + source_session_id + source_run_id`;
- backup restore is operator-only, transactional and fail-closed;
- reboot never auto-continues restore/apply;
- persisted restore evidence blocks new backup/restore until explicit cleanup;
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

Key areas:

```text
Core/
Arduino/
firmware/arduino/src/main.cpp
Arduino/Config/CM_Pins.h
```

### ESP32 owns service/data/UI orchestration

- Wi-Fi/AP/mDNS/HTTP/FTP
- microSD and RTC
- clients/motors/repairs
- winding job preparation and persistence
- warehouse/materials/costing
- event journal
- backup/restore
- web UI and diagnostics

Main integration point:

```text
firmware/esp32/src/main.cpp
```

Web/API service composition also exists in:

```text
firmware/esp32/src/CM_StaticSiteServer.h/.cpp
```

## Protocol boundary

Production UART is text `CMP1|...`:

```text
Arduino/CM_UartEventTransport.h/.cpp
firmware/esp32/src/CM_UartEventReceiver.h/.cpp
Shared/CMP1Text/CM_Cmp1Crc.h
```

`Shared/Protocol/` is the older binary CMP host-test-only protocol and is **not** a drop-in replacement for production CMP1.

Any production UART frame change must be reviewed on both boards, in tests, compatibility rules and documentation.

## Fast change rule

Before coding, identify all five parts of the change:

```text
OWNER → CONTRACT → PERSISTENCE → UI/API → VERIFICATION
```

If one is not applicable, state why. Do not add a hidden subsystem with no clear owner or lifecycle.

For detailed routes by task, use `docs/AI_AGENT/02_CHANGE_ROUTER.md`.

## Documentation rule

When a production capability is added or moved:

- update the relevant `docs/AI_AGENT/` map/router entry;
- update the thematic project doc when a public/data/protocol contract changes;
- update handoff only when the project state/release baseline materially changes.

The goal is that the next agent can locate the change without repository-wide archaeology.
