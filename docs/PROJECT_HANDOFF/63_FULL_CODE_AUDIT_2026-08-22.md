# Checkpoint 63 — Full code audit phase

Date: 2026-08-22
Branch: `cmp-protocol-v1`

## Purpose

Checkpoint 62 is the current verified repo-level safety/integrity baseline. The update/hardening plan is closed at repo level.

The remaining two-board UART/hardware smoke is an external verification gate, not an active software-development backlog item. It remains required when the physical stand is available, but it must not block repo-level review or cause old completed tasks to be reopened.

The new active phase is a full audit of the current `cmp-protocol-v1` codebase for defects, weak contracts, unsafe state transitions, persistence inconsistencies, resource risks, stale/dead implementation, and missing regression coverage.

## Verified baseline entering the audit

Production ESP32 C++ baseline:

```text
5fa6bcea812c33f0b2dc8e13baae476221839b3a
Validate session state against repeat target
```

Verified automated gates:

```text
ESP32 Build #1245 — GREEN
run 32515224487
head_sha 5fa6bcea812c33f0b2dc8e13baae476221839b3a

CMP Protocol Tests #2210 — GREEN
run 32515361340
head_sha ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133
```

Do not claim physical hardware acceptance from these CI results.

## External verification gate retained

When the hardware stand is available, run targeted two-board smoke:

```text
ESP32 JOB -> Arduino READY
physical START only
RUN_STARTED
RUN_COMPLETED
event ACK/replay
final repeat remains terminal
no automatic wire/material writeoff

no-run JOB -> cancel
ALREADY_CLEAR
physical D -> * -> # -> D -> ALL_CLEAR when safe
lost JOB_ACK / timeout -> manual review
late ALL_CLEAR after completed job -> no persisted corruption/storage fault
```

This gate is retained but is not the active repo-level work queue.

## Full audit scope

### A. Arduino safety and realtime machine ownership

Review `Core/`, `Arduino/`, `firmware/arduino/src/` for physical START/SSR/Hall ownership, repeat/run identity, remote JOB lifecycle, UART validation/retry/replay, RAM/Flash/stack, blocking paths and reboot/fault behavior.

### B. ESP32 runtime, persistence and APIs

Review all `firmware/esp32/src/*.cpp` and headers for lifecycle/state consistency, persistence identities, workshop/warehouse/writeoff/costing, backup/restore, network/FTP/RTC/SD failures, API semantics, bounds/overflow, atomic writes, NDJSON cost and duplicate ownership.

### C. Desktop/mobile web

Review `firmware/esp32/web/desktop/`, `mobile/`, shared assets for API parity, error handling, unsafe optimistic state, paging/bounds, provenance, confirmations, semantic divergence and injection/escaping risks.

### D. Tests and CI

Review `Tests/`, `.github/workflows/`, `platformio.ini` and build scripts for actual production coverage, stale/false-positive assertions, fail-open steps, source-filter omissions and workflow trigger gaps.

### E. Documentation/AI routing

Review authoritative entrypoints for current-code accuracy, safety ownership, verified baseline and stale-task routing. Historical checkpoints remain history.

## Audit severity

```text
P0 — immediate physical/data safety risk or destructive corruption path
P1 — serious functional/state/persistence bug likely to affect production
P2 — real robustness/performance/maintainability weakness with concrete failure mode
P3 — cleanup/dead code/documentation/test-quality issue with low runtime risk
```

Do not label style preferences or speculative redesigns as defects.

## Findings

### A-001 — P1 — remote JOB could overwrite non-idle Arduino state — FIXED, CI PENDING

Observed in `Core/CM_StateMachine.cpp` before fix:

- `loadRemoteJob()` rejected only `Winding`, `Paused` and `ManualRun`;
- a new remote JOB could therefore replace local `EnterTurns`, `CoilComplete`, `JobComplete`, `Fault`, or another accepted `Ready` job;
- `session_id == 0` was silently replaced with an Arduino-local allocated session id even though remote provenance must stay exact across boards.

This could lose a partially executed/local program or break ESP32<->Arduino job/session identity without physically starting the motor by itself.

Fix:

```text
20f422dd8170139ed7caac6159edb23fe7775103
Harden Arduino remote job admission
```

New rules:

- remote `job_id` and `session_id` must both be non-zero;
- a new remote JOB is accepted only from truly empty `EnterCoilCount` HOME state;
- an exact duplicate of the already accepted zero-run remote `Ready` job is idempotently accepted for lost-ACK retry;
- any different JOB cannot overwrite local entry, partial run, completed job, fault, or accepted remote identity.

Regression coverage:

```text
a95a73ac86aab061475f3f2cf88d8bbd6b3de837
Guard remote job admission boundaries
Tests/Protocol/test_repeat_target.cpp
```

Required verification after this production Arduino change:

```text
Arduino Uno Build — NOT VERIFIED
CMP Protocol Tests — NOT VERIFIED
hardware remote JOB smoke — external gate
```

### A-002 — P2 — production JOB parser accepts non-canonical tokens — CONFIRMED, OPEN

`Arduino/CM_UartEventTransport.cpp::parseRemoteJob()` currently uses permissive `strtoul(..., nullptr, 10)` for several numeric fields and maps any non-`STARTING` type token to `WORKING`.

Therefore a CRC-valid but syntactically malformed frame can be interpreted instead of rejected fail-closed, e.g. numeric tokens with trailing characters or an unknown winding type.

State-machine hardening from A-001 now prevents zero/mismatched identity from overwriting active state, but the wire parser itself still requires strict canonical validation and regression coverage.

This is the current Phase A code target.

## Execution rules

For every existing file changed:

1. fetch current content from `cmp-protocol-v1`;
2. use current blob SHA;
3. make the smallest safe fix;
4. add/extend regression coverage where practical;
5. update this checkpoint/current-state docs only after code meaning changes;
6. never claim GREEN until the named workflow actually passes.

Safety invariants remain unchanged:

```text
physical START only
no automatic START between repeats
no auto-resume after reboot
Arduino owns SSR
ESP32/Web never directly drive SSR
RUN_COMPLETED never auto-writes off material
manual writeoff uses exact source_session_id + source_run_id
spool_id optional only for approved KG_FIRST unallocated/manual path
exact spool provenance retained when a spool is used
backup restore operator-only, transactional and fail-closed
no automatic production-data cleanup
```

## Audit status

```text
Phase A inventory / cross-cutting ownership: IN PROGRESS
Arduino audit: IN PROGRESS
  A-001 FIXED / CI PENDING
  A-002 CONFIRMED / OPEN
ESP32 audit: PENDING
Web audit: PENDING
Tests/CI audit: PENDING
Documentation/AI consistency audit: PENDING
Final repo-wide recheck: PENDING
```

Completed findings must not remain in `06_ACTIVE_WORK_AND_NEXT_STEPS.md` as future work.
