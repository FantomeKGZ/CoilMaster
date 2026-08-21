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

After `5fa6bcea`, later commits before checkpoint closure changed tests/documentation only until the explicit ESP32 build-trigger marker update. Do not claim physical hardware acceptance from these CI results.

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

Audit the repository in these layers. Findings are fixed as they are confirmed; do not accumulate speculative rewrites.

### A. Arduino safety and realtime machine ownership

Review:

- `Core/`
- `Arduino/`
- `firmware/arduino/src/`

Check:

- physical START ownership;
- SSR ownership and safe release;
- Hall counting/calibration boundaries;
- repeat execution and run identity;
- remote JOB acceptance/cancel/recovery;
- UART parsing, retry, duplicate/replay handling;
- RAM/Flash/stack-sensitive code;
- blocking paths in realtime loop;
- invalid-state and reboot behavior.

### B. ESP32 runtime, persistence and APIs

Review all `firmware/esp32/src/*.cpp` and headers.

Check:

- job lifecycle/state transitions;
- snapshot/state/journal consistency;
- repair/motor/workshop persistence;
- warehouse/material/writeoff provenance;
- costing/finalization;
- backup/restore/integrity;
- network/FTP/RTC/SD failure semantics;
- API status/error codes and identity checks;
- allocation/overflow/bounds;
- temporary-file and atomic-write behavior;
- growing NDJSON/read-path cost;
- hidden duplicate implementations or ownership overlap.

### C. Desktop/mobile web

Review:

- `firmware/esp32/web/desktop/`
- `firmware/esp32/web/mobile/`
- shared web assets/helpers where present.

Check:

- API contract parity;
- stale/dead endpoints;
- error handling;
- unsafe optimistic UI state;
- pagination/bounds;
- identity/provenance display;
- destructive actions and confirmations;
- desktop/mobile semantic divergence;
- injection/escaping risks in generated HTML/DOM.

### D. Tests and CI

Review:

- `Tests/`
- `.github/workflows/`
- `platformio.ini`
- build scripts.

Check:

- production paths actually compiled/tested;
- missing contract coverage;
- stale assertions;
- false-positive static source checks;
- skipped/fail-open steps;
- build filter omissions;
- workflow path filters that fail to trigger relevant CI.

### E. Documentation/AI routing

Review authoritative entrypoints only for current-code accuracy. Historical checkpoints remain history.

Check:

- no stale active task;
- no contradictory safety ownership;
- correct current verified baseline;
- accurate source-of-truth file locations;
- no AI instructions that can reopen completed work.

## Audit severity

Use:

```text
P0 — immediate physical/data safety risk or destructive corruption path
P1 — serious functional/state/persistence bug likely to affect production
P2 — real robustness/performance/maintainability weakness with concrete failure mode
P3 — cleanup/dead code/documentation/test-quality issue with low runtime risk
```

Do not label style preferences or speculative redesigns as defects.

## Execution rules

For every existing file changed:

1. fetch current content from `cmp-protocol-v1`;
2. use current blob SHA;
3. make the smallest safe fix;
4. add/extend regression coverage where practical;
5. update this checkpoint/current-state docs only after the code meaning changes;
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
Arduino audit: PENDING
ESP32 audit: PENDING
Web audit: PENDING
Tests/CI audit: PENDING
Documentation/AI consistency audit: PENDING
Final repo-wide recheck: PENDING
```

Findings and fixes must be recorded here or in subsequent numbered audit checkpoints. Completed findings must not remain in `06_ACTIVE_WORK_AND_NEXT_STEPS.md` as future work.
