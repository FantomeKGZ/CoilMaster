# CoilMaster — AI maintenance start guide

Purpose: let a new AI/coding agent understand where to make a change in minutes, without scanning the whole repository or relying on stale chat history.

## 1. Current status

CoilMaster v1 is a stable **RELEASE READY** baseline. The authoritative release checkpoint is:

```text
docs/PROJECT_HANDOFF/38_COILMASTER_V1_RELEASE_READY_2026-08-16.md
```

Closed hardware gates must not be repeated merely because documentation/tests change. Re-run only verification that is relevant to production code actually modified.

## 2. Source precedence

When information conflicts, use this order:

1. current code in `cmp-protocol-v1`;
2. actual build/test/hardware result;
3. `docs/AI_AGENT/` navigation docs for locating the current implementation;
4. release checkpoint `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md`;
5. thematic docs and older handoff checkpoints.

`main` is not an implementation source.

## 3. Five-minute mental model

```text
                         SERVICE / DATA / UI
┌──────────────────────────────────────────────────────────────┐
│ ESP32                                                        │
│ Wi-Fi • HTTP • microSD • RTC • registry • warehouse • backup│
│ job preparation • persistence • winding event journal       │
└──────────────────────────────┬───────────────────────────────┘
                               │ CMP1 UART
                               │ job ↓   events ↑
┌──────────────────────────────▼───────────────────────────────┐
│ Arduino Uno                                                  │
│ physical START • Hall • state machine • SSR • LCD/keypad     │
└──────────────────────────────────────────────────────────────┘

Production flow:
client → motor → OPEN repair → costing → linked winding → exact spool
→ immutable snapshot/spool selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual exact-run wire writeoff
→ finalization preflight → CLOSED → reports → backup
```

The ESP32 may prepare/deliver a job, but it does not physically start the machine and does not directly own SSR.

## 4. Where to go next

### Need to understand the whole project layout?
Open:

```text
01_PROJECT_MAP.md
```

It maps hardware ownership, source trees, major modules, persistence and integration points.

### Know what you want to change but not where?
Open:

```text
02_CHANGE_ROUTER.md
```

It maps common tasks to the first files to inspect, coupled contracts and expected verification.

### Adding a new module/service/page/storage domain?
Open:

```text
03_ADD_MODULE_PLAYBOOK.md
```

It defines ownership, lifecycle, registration, persistence, API/UI and test checklist.

### Need to know which build/test/hardware gates apply?
Open:

```text
04_VERIFICATION_MATRIX.md
```

## 5. Critical integration points

### Arduino production entrypoint

```text
firmware/arduino/src/main.cpp
```

Compiled together with:

```text
Core/*.cpp
Arduino/*.cpp
```

### ESP32 production entrypoint

```text
firmware/esp32/src/main.cpp
```

This owns many top-level production service objects and the main job lifecycle.

### ESP32 web/static service composition

```text
firmware/esp32/src/CM_StaticSiteServer.h
firmware/esp32/src/CM_StaticSiteServer.cpp
```

Some HTTP modules are owned here rather than directly in `main.cpp`.

### Production web assets

```text
firmware/esp32/web/mobile/
firmware/esp32/web/desktop/
firmware/esp32/web/shared/
```

Substantial UI changes normally require mobile + desktop parity.

### Production UART

```text
Arduino/CM_UartEventTransport.h/.cpp
firmware/esp32/src/CM_UartEventReceiver.h/.cpp
Shared/CMP1Text/CM_Cmp1Crc.h
```

Do not substitute `Shared/Protocol/`; it is an older binary protocol used by host tests.

## 6. Change procedure for an AI agent

For every task:

1. Confirm branch `cmp-protocol-v1` and current HEAD.
2. Use `02_CHANGE_ROUTER.md` to find the smallest relevant file set.
3. Fetch every existing target file immediately before editing and keep its current blob SHA.
4. Inspect owner + contract + persistence + UI/API + tests before changing behavior.
5. Make the smallest coherent change.
6. Add/update tests that protect the changed contract.
7. Run the relevant verification from `04_VERIFICATION_MATRIX.md`.
8. Update the AI map/router if a component was added/moved or ownership changed.
9. Only update release/handoff state when the actual project/release state changed.

## 7. Safety stop signs

If a proposed change would do any of the following, do not implement it as a convenience shortcut:

- remote/automatic physical START;
- direct ESP32/Web SSR control;
- automatic resume after reboot;
- automatic wire deduction from `RUN_COMPLETED`;
- writeoff without exact spool/session/run provenance;
- automatic restore/apply after reboot;
- bypass of persisted restore stale evidence;
- automatic deletion of production data when storage fills.

These are release safety contracts, not implementation details.

## 8. Existing detailed project documentation

Important thematic references:

```text
ARCHITECTURE.md
docs/PROJECT_HANDOFF/02_ARCHITECTURE_AND_HARDWARE.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/PROJECT_HANDOFF/04_DATA_STORAGE_API_UI.md
docs/PROJECT_HANDOFF/08_WORK_RULES_AND_VERIFICATION.md
docs/PROJECT_HANDOFF/09_KEY_FILES_INDEX.md
docs/MOTOR_IMPORT_FORMAT.md
docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md
docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
```

Use them for detail after the AI map has narrowed the relevant subsystem.
