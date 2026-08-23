# CoilMaster — AI maintenance start guide

Purpose: let a new AI/coding agent understand the current project without scanning historical handoffs or reviving closed work.

## 1. Current status

Authoritative current transition/active documents:

```text
docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
```

Latest operator-confirmed implementation/test GREEN baseline:

```text
ad17bb7029f9f0f694fcb275ce729d0c23c8e1dd
Harden release safety contract against stale JSON assertions
CMP Protocol Tests GREEN
```

Exact Actions evidence is recorded in `06_ACTIVE_WORK_AND_NEXT_STEPS.md`. Documentation-only commits after that SHA do not create a new firmware GREEN baseline. Later implementation changes require their own workflow result or later explicit operator confirmation.

The targeted ESP32<->Arduino hardware smoke remains a separate external verification gate for when the physical stand is needed. It is not inferred from CI and is not counted as software cleanup debt.

The full `cmp-protocol-v1` audit sections A..E are complete. The current software phase is the final controlled repository cleanup/zero-debt sweep. The active handoff estimates that cleanup at about 96% complete; use `06_ACTIVE_WORK_AND_NEXT_STEPS.md` for the current remaining queue rather than copying the percentage forward blindly after new changes.

Cleanup/de-duplication is allowed only after direct dependency proof. Every candidate must be classified `DELETE`, `MERGE`, `KEEP` or `REVIEW`; uncertain dependencies stay in place.

## 2. Source precedence

When information conflicts, use this order:

1. current code in `cmp-protocol-v1`;
2. actual build/test/hardware result;
3. `docs/PROJECT_HANDOFF/00_READ_FIRST.md`;
4. `docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md`;
5. `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`;
6. relevant current thematic checkpoint such as `64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md`;
7. `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`;
8. `docs/AI_AGENT/` navigation docs;
9. thematic docs;
10. older numbered handoff checkpoints.

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
-> immutable job snapshot + exact spool selection -> UART
-> physical START -> RUN_STARTED/RUN_COMPLETED
-> manual exact-run exact-spool material writeoff
-> finalization -> CLOSED -> reports -> backup
```

The ESP32 may prepare/deliver a job, but it does not physically start the machine and does not directly own SSR.

For current linked production KG-first consumption, exact `source_session_id + source_run_id + immutable spool_id` provenance is mandatory. Historical `UNALLOCATED` KG_FIRST records remain readable/auditable/recoverable as compatibility evidence only; they do not permit a new linked production writeoff to omit an already selected spool. If a true unallocated production workflow is ever required, its material provenance must become immutable before the UART boundary.

## 4. Where to go next

### Need the project layout?
Open `01_PROJECT_MAP.md`.

### Know the task but not the files?
Open `02_CHANGE_ROUTER.md`.

### Adding a module/service/page/storage domain?
Open `03_ADD_MODULE_PLAYBOOK.md`.

### Need build/test/hardware routing?
Open `04_VERIFICATION_MATRIX.md`.

### Need active work / continuation in a new chat?
Open:

```text
docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md
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
CM_ConductorSettingsStore.cpp
WarehouseStore::loadConversionSettings / setConversionSettings
/data/settings/conductor.json
```

The old parallel `CM_ConductorSettings.*` persistence implementation has already been removed during controlled cleanup. Do not reintroduce it as a second production owner.

## 6. Current work order

The repo-level audit is complete. Continue the final controlled cleanup unless a concrete higher-severity defect redirects the work:

```text
A. Arduino safety/realtime/UART/resources                  COMPLETE
B. ESP32 runtime/API/persistence/integrity/network/backup COMPLETE
C. desktop/mobile/shared Web parity/error/security        COMPLETE
D. tests/CI/build filters/path triggers                   COMPLETE
E. docs/AI routing consistency                            COMPLETE
F. controlled cleanup/de-duplication                      ACTIVE (~96% at current handoff)
G. separate final hardware smoke when source-level work requires it
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
2. Read `00_READ_FIRST.md` + handoff 67 + active queue 06.
3. Use `02_CHANGE_ROUTER.md` to find the smallest relevant file set.
4. Fetch every existing target file immediately before editing and keep its current blob SHA.
5. Inspect owner + contract + persistence + UI/API + tests before changing behavior.
6. Make the smallest coherent change.
7. Add/update tests that protect the changed contract.
8. Run the relevant verification from `04_VERIFICATION_MATRIX.md`.
9. Update AI map/router only if topology/ownership/contract location changed.
10. Keep `06_ACTIVE_WORK_AND_NEXT_STEPS.md` and `67_NEXT_CHAT_HANDOFF_2026-08-22.md` synchronized when status materially changes.

For cleanup, never delete merely because a file looks old. First prove includes/imports, PlatformIO ownership, workflow/test references, runtime file paths, web injection/references and docs/AI routing. Classify each candidate DELETE/MERGE/KEEP/REVIEW. Empty GitHub code search is supporting evidence only, never sufficient proof by itself.

## 8. Safety stop signs

Do not implement convenience shortcuts that create:

- remote/automatic physical START;
- automatic START between repeats;
- direct ESP32/Web SSR control;
- automatic resume after reboot;
- automatic material deduction from `RUN_COMPLETED`;
- material writeoff without exact source session/run provenance;
- loss or omission of the immutable exact spool provenance for a current linked production run;
- post-run downgrade of an exact-spool session into `UNALLOCATED`;
- automatic restore/apply after reboot;
- bypass of persisted restore stale evidence;
- automatic deletion/truncation of production data when storage fills.

These are safety contracts, not implementation details.

## 9. External hardware gate

When hardware is needed, perform the targeted two-board smoke defined by the active handoff/checkpoints. Hardware GREEN must never be inferred from CI. Do not request broad runtime logs while the remaining issue can still be resolved from source/tree evidence.
