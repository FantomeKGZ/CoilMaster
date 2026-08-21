# Checkpoint 63 — Full code audit phase

Date: 2026-08-22
Branch: `cmp-protocol-v1`

## Purpose

Checkpoint 62 established the repo-level safety/integrity baseline that existed when this audit began.

The audit was temporarily paused to finish the remaining Phase 9 work from the approved 2026-08-20 UI/hardware/job-lifecycle plan. That implementation is now closed by:

```text
docs/PROJECT_HANDOFF/64_PHASE9_SHARED_WEB_SHELL_AND_SEARCH_2026-08-22.md
```

Current Phase 9/newer HEAD CI is still `NOT VERIFIED` until an exact Actions run with matching `head_sha` is inspected. This does not reopen Phase 9 implementation as active backlog.

The active repo-level phase is again a full audit of the current `cmp-protocol-v1` codebase for defects, weak contracts, unsafe state transitions, persistence inconsistencies, resource risks, stale/dead implementation, and missing regression coverage.

## Verified baseline entering the audit

Previously verified automated gates:

```text
ESP32 Build #1245 — GREEN
run 32515224487
head_sha 5fa6bcea812c33f0b2dc8e13baae476221839b3a

CMP Protocol Tests #2210 — GREEN
run 32515361340
head_sha ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133
```

These older successful runs do not prove the newer Phase 9/current HEAD green. Do not claim physical hardware acceptance from CI.

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

This gate is retained but does not block repo-level review.

## Targeted desync/recovery review retained after Phase 9

In addition to the normal audit sequence, explicitly review the cross-board scenario that motivated the temporary recovery investigation:

```text
ESP32 believes a JOB was sent/pending
Arduino does not visibly hold the expected JOB
operator tries to cancel/clear
both boards must converge without inventing RUN completion or corrupting persisted state
```

Trace current transitions for:

```text
JOB / JOB_ACK
JOB_CANCEL / JOB_CANCEL_ACK
ALREADY_CLEAR
ALL_CLEAR
timeout / retry
late ACK / late ALL_CLEAR
ESP32 reboot
Arduino reboot
replay
persisted ESP32 job state
```

The generic cancel/recovery feature already exists; this is a targeted defect audit, not an instruction to reimplement it. Change code only if a concrete inconsistent/fail-open transition is proven.

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

Required verification after this production Arduino change remains separate from older green evidence:

```text
Arduino Uno Build — NOT VERIFIED here
CMP Protocol Tests for exact change/head — NOT VERIFIED here
hardware remote JOB smoke — external gate
```

### A-002 — P2 — production JOB parser accepted non-canonical tokens — FIXED, CI PENDING

Before the fix, `Arduino/CM_UartEventTransport.cpp::parseRemoteJob()` used permissive `strtoul(..., nullptr, 10)` for `job_id`, `session_id`, coil count and turn values, while any non-`STARTING` type token became `WORKING`.

A CRC-valid but malformed JOB could therefore be interpreted rather than rejected fail-closed.

Fix commits:

```text
baa028cf7bc72a5d7b0e523cc33013cb4050e677
Reject non-canonical Arduino JOB frames

699116497c3614bc2a8fc621bc326d686d98f221
Fix bounded canonical integer parsing
```

Current wire rules:

- unsigned decimal tokens are full-token digits only;
- signs, whitespace, suffixes and leading-zero non-canonical forms are rejected;
- per-field upper bounds and uint32 overflow are checked before assignment/narrowing;
- remote `job_id` and `session_id` must be non-zero at the wire boundary;
- repeat target, coil count and every turns token use the same bounded canonical parser;
- only exact `STARTING` and `WORKING` type tokens are accepted;
- CRC/capability handling and normal CMP1 JOB layout are unchanged.

Compatibility check against current ESP32 sender:

`CM_UartEventReceiver.cpp::writeJobFrame()` emits `%lu/%u`, exact `STARTING`/`WORKING`, `R%u` and `C`; therefore current normal ESP32 frames remain canonical under the stricter Arduino parser.

Regression guard:

```text
583d9275519058029ff9477ffb1b2ec894ae9fa6
Guard strict Arduino JOB parsing
Tests/Protocol/check_arduino_job_parser_contracts.js

7d4a5cadda6dd3971a182d9fb20fced48accfb01
Run Arduino JOB parser audit in CI
.github/workflows/cmp-protocol-tests.yml
```

Current verification:

```text
repo implementation: FIXED
source regression guard: PRESENT
current exact CMP Protocol Tests run: NOT VERIFIED
current exact Arduino Uno Build: NOT VERIFIED
```

Do not promote this to CI-verified until exact matching workflow results are inspected.

## Current targeted review — UART desync/recovery

With A-002 fixed at repo level, continue immediately through current inbound/outbound UART correlation and persisted state:

1. strict/canonical validation of ACK/NACK and JOB_CANCEL paths;
2. JOB retry -> cancel handoff after possible lost JOB_ACK;
3. `ALREADY_CLEAR` and physical `ALL_CLEAR` correlation;
4. late ACK / late ALL_CLEAR after state transitions;
5. ESP32 and Arduino reboot behavior;
6. persisted ESP32 job-state transition consistency.

Record new findings as A-003/B-* depending on ownership/severity; do not reimplement generic cancellation unless a concrete defect is proven.

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
Approved plan Phase 9 implementation: COMPLETE (checkpoint 64)
Current newer HEAD CI: NOT VERIFIED
Phase A inventory / cross-cutting ownership: IN PROGRESS
Arduino audit: IN PROGRESS
  A-001 FIXED / exact-current verification pending
  A-002 FIXED / CI PENDING
Targeted JOB/JOB_CANCEL/ALL_CLEAR desync audit: IN PROGRESS / CURRENT TARGET
ESP32 audit: PENDING
Web audit: PENDING
Tests/CI audit: PENDING
Documentation/AI consistency audit: PENDING
Final repo-wide recheck: PENDING
```

Completed findings must not remain in `06_ACTIVE_WORK_AND_NEXT_STEPS.md` as future work.
