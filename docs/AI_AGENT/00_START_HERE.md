# CoilMaster — AI maintenance start guide

Purpose: let a new AI/coding agent understand the current project without scanning historical handoffs or reviving closed work.

## 1. Current status

Authoritative current active documents:

```text
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

Latest operator-confirmed green baseline at the time of this guide update:

```text
3ebc942f1be9397af9d8ee5336c0ed78e9b13c87
Record calculator standard alternatives feature
USER CONFIRMED GREEN
```

Commits after that SHA require their own workflow result or a later explicit operator confirmation. Older verified workflow run IDs remain historical evidence, not the current implementation baseline.

The targeted ESP32<->Arduino hardware smoke remains an external verification gate for when the physical stand is available. It is not the current software backlog.

The current software phase is a full audit of the `cmp-protocol-v1` codebase. Confirmed defects are fixed as they are found; completed historical features are not reopened merely because they are being audited.

After sections A..E and the final cross-layer recheck are complete, the user has explicitly approved a separate repository cleanup/de-duplication phase. Cleanup starts only after dependency inventory proves each deletion/merge safe.

## 2. Source precedence

When information conflicts, use this order:

1. current code in `cmp-protocol-v1`;
2. actual build/test/hardware result;
3. `docs/PROJECT_HANDOFF/00_READ_FIRST.md`;
4. `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`;
5. `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`;
6. `docs/AI_AGENT/` navigation docs;
7. thematic docs;
8. older numbered handoff checkpoints.

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
client -> motor -> OPEN repair -> costing -> linked winding
-> immutable job snapshot/material selection -> UART
-> physical START -> RUN_STARTED/RUN_COMPLETED
-> manual exact-run material writeoff
-> finalization -> CLOSED -> reports -> backup
```

The ESP32 may prepare/deliver a job, but it does not physically start the machine and does not directly own SSR.

For new KG-first material consumption, exact `source_session_id + source_run_id` provenance is mandatory. `spool_id` may be absent only in the approved unallocated/manual KG-first path; exact spool provenance remains mandatory whenever a spool is used.

## 4. Where to go next

### Need the project layout?
Open `01_PROJECT_MAP.md`.

### Know the task but not the files?
Open `02_CHANGE_ROUTER.md`.

### Adding a module/service/page/storage domain?
Open `03_ADD_MODULE_PLAYBOOK.md`.

### Need build/test/hardware routing?
Open `04_VERIFICATION_MATRIX.md`.

### Need active work?
Open:

```text
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

Do not use an old checkpoint's `next` section as active work.

## 5. Critical integration points

Arduino production entrypoint:

```text
firmware/arduino/src/main.cpp
```

Compiled with `Core/*.cpp` and `Arduino/*.cpp`.

ESP32 production entrypoint:

```text
firmware/esp32/src/main.cpp
```

ESP32 web/static composition:

```text
firmware/esp32/src/CM_StaticSiteServer.h
firmware/esp32/src/CM_StaticSiteServer.cpp
```

Production web assets:

```text
firmware/esp32/web/mobile/
firmware/esp32/web/desktop/
firmware/esp32/web/shared/
```

Production UART:

```text
Arduino/CM_UartEventTransport.h/.cpp
firmware/esp32/src/CM_UartEventReceiver.h/.cpp
Shared/CMP1Text/CM_Cmp1Crc.h
```

Do not substitute `Shared/Protocol/`; it is the older binary host-test protocol.

Production conductor-calculator settings currently belong to:

```text
CM_ConductorSettingsWeb.*
WarehouseStore::loadConversionSettings / setConversionSettings
/data/settings/conductor.json
```

The parallel legacy `CM_ConductorSettings.*` persistence implementation is not the production owner and is a post-audit cleanup candidate pending dependency proof.

## 6. Current audit order

Audit in this order unless a concrete higher-severity defect redirects the work:

```text
A. Arduino safety/realtime/UART/resources
B. ESP32 runtime/API/persistence/integrity/network/backup
C. desktop/mobile/shared Web parity/error/security
D. tests/CI/build filters/path triggers
E. docs/AI routing consistency
F. final cross-layer recheck + applicable CI
G. post-audit repository cleanup/de-duplication
```

Severity:

```text
P0 physical/data safety or destructive corruption
P1 serious functional/state/persistence defect
P2 concrete robustness/performance/maintainability weakness
P3 low-risk cleanup/dead code/docs/test-quality issue
```

Do not manufacture findings from style preferences or speculative redesigns.

## 7. Change procedure

1. Confirm branch `cmp-protocol-v1` and current HEAD.
2. Read `00_READ_FIRST.md` + checkpoint 63 + active queue 06.
3. Use `02_CHANGE_ROUTER.md` to find the smallest relevant file set.
4. Fetch every existing target file immediately before editing and keep its current blob SHA.
5. Inspect owner + contract + persistence + UI/API + tests before changing behavior.
6. Make the smallest coherent change.
7. Add/update tests that protect the changed contract.
8. Run the relevant verification from `04_VERIFICATION_MATRIX.md`.
9. Update AI map/router only if topology/ownership/contract location changed.
10. Update current handoff state only when project status materially changed.

For cleanup, never delete merely because a file looks old. First prove includes/imports, PlatformIO ownership, workflow/test references, runtime file paths, web injection/references and docs/AI routing. Classify each candidate DELETE/MERGE/KEEP/REVIEW.

## 8. Safety stop signs

Do not implement convenience shortcuts that create:

- remote/automatic physical START;
- automatic START between repeats;
- direct ESP32/Web SSR control;
- automatic resume after reboot;
- automatic material deduction from `RUN_COMPLETED`;
- material writeoff without exact source session/run provenance;
- loss of exact spool provenance when a spool is used;
- automatic restore/apply after reboot;
- bypass of persisted restore stale evidence;
- automatic deletion of production data when storage fills.

These are safety contracts, not implementation details.

## 9. External hardware gate

When hardware is available, perform the targeted two-board smoke defined by checkpoint 63/65. Hardware GREEN must never be inferred from CI.
