# CoilMaster — current recovery and documentation baseline

Date: 2026-08-21
Branch: `cmp-protocol-v1`

This checkpoint supersedes older handoff files as the current recovery/status reference. Older numbered checkpoints remain historical evidence only and must not be treated as active work unless this file or `00_READ_FIRST.md` explicitly reopens them.

## Current verified code baseline

Production fix commit:

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
Fix duplicate ESP32 job lifecycle definitions
```

Verified GitHub Actions on that exact commit:

```text
CMP Protocol Tests #2175 — GREEN
run 32499582610

ESP32 Build #1241 — GREEN
run 32499582503
```

The ESP32 build failure before this commit was a linker failure caused by duplicate definitions of:

```text
JobStateStore::closeAfterRemoteCancel(...)
JobStateStore::dismissInactive(...)
```

The obsolete duplicate `firmware/esp32/src/CM_JobStateStoreLifecycle.cpp` was removed. The authoritative implementations remain in:

```text
firmware/esp32/src/CM_JobStateRemoteCancel.cpp
firmware/esp32/src/CM_JobStateDismiss.cpp
```

## JOB cancel / recovery status

This feature is IMPLEMENTED and CLOSED at repo level. Do not reopen it as a generic next task without a concrete regression.

Current Arduino behavior includes:

- no-run remote JOB cancellation;
- idempotent cancel when Arduino is already clear (`ALREADY_CLEAR`);
- physical fallback sequence `D -> * -> # -> D`;
- fallback emits `ALL_CLEAR` only after proving there is no active physical run;
- it never emits `RUN_COMPLETED` and never implies a completed winding;
- reboot recovery does not auto-start the machine and does not auto-writeoff wire.

ESP32 keeps cancellation/recovery separated from immutable historical run evidence. Operational cancellation must not erase completed run history or linked immutable evidence.

## Safety invariants

Do not weaken:

- physical START is local/physical only;
- no automatic START between repeat cycles;
- no auto-resume after reboot;
- ESP32/Web never control SSR directly;
- `RUN_COMPLETED` never performs automatic wire writeoff;
- new KG-first manual consumption always requires exact `source_session_id + source_run_id` provenance;
- `spool_id` is optional only in the approved KG-first unallocated/manual path;
- legacy exact-spool provenance remains supported and exact when a spool is used;
- backup restore remains operator-only, transactional and fail-closed;
- hardware settings/calibration remain safe-idle gated.

## Current implemented production flow

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> immutable job snapshot/material selection
-> UART JOB delivery -> physical START
-> RUN_STARTED/RUN_COMPLETED
-> explicit manual exact-run material writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

Repeat semantics remain one JOB with one physical START per run/repeat. No repeat auto-start exists.

## Documentation precedence

For a new AI/coding session use this order:

```text
1. /AGENTS.md
2. docs/PROJECT_HANDOFF/00_READ_FIRST.md
3. this checkpoint (61)
4. docs/AI_AGENT/00_START_HERE.md
5. docs/AI_AGENT/01_PROJECT_MAP.md
6. docs/AI_AGENT/02_CHANGE_ROUTER.md
7. docs/AI_AGENT/04_VERIFICATION_MATRIX.md
8. current production code
```

When prose conflicts with code or an actual verification result, current code + actual verification win.

Older checkpoints (`38`, `24`, etc.) are historical baselines. They must not be used to select active work.

## Active work after this checkpoint

Repo-level protocol and ESP32 build recovery are closed.

Next work should be selected only from current evidence:

1. verify current Arduino Uno Build after recent code/history changes;
2. perform targeted two-board UART/hardware smoke acceptance for the current production head when hardware is available;
3. address only concrete failures, measured performance hotspots or explicitly requested product work;
4. do not restart closed archive/pagination/backup/kg-first/JOB-cancel tasks just because an older checkpoint lists them as next steps.

Hardware acceptance is a separate evidence class and is not implied by green GitHub Actions.
