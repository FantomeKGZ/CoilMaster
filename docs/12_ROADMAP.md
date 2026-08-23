# CoilMaster — дорожная карта

## Статус

Исторические этапы архитектуры, Arduino Core, CMP1 integration, ESP32 persistence/API, desktop/mobile Web, warehouse/materials/costing, backup/restore и controlled cleanup в основном уже реализованы в `cmp-protocol-v1`.

Этот файл больше не является списком незавершённых v0.x задач. Активная очередь и verified GREEN baseline всегда берутся из:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

## Завершённые software blocks

- Arduino realtime state machine, Hall, physical START, SSR, keypad/LCD/buzzer;
- production CMP1 UART JOB/ACK/cancel/run-event transport;
- lost-ACK/timeout and no-run recovery semantics;
- ESP32 microSD/RTC/network/Web/API orchestration;
- clients/motors/repairs and motor import/reference workflows;
- immutable winding job snapshot/state/exact spool selection;
- winding journal and autonomous local winding archive;
- wire spool warehouse, auxiliary materials, pricing/costing and manual exact-run writeoff;
- repair finalization preflight and persisted CLOSED evidence;
- desktop/mobile/shared Web UI and reference site;
- deep persistence/integrity audit, backup/export/restore/rollback safety;
- CI build/test routing and extensive source/static contract regression suite;
- full repo audit A–E and controlled zero-debt cleanup.

## Current release path

### 1. Finish controlled repository cleanup

For every remaining candidate:

```text
DELETE / MERGE / KEEP / REVIEW
```

Deletion requires direct owner/build/runtime/test/docs proof. Empty code search alone is never sufficient.

Remaining work is limited to final owner-by-owner and stale-contract checks plus handoff consolidation. Do not reopen already completed architecture/provenance/residue audits without current-source evidence.

### 2. Keep applicable CI GREEN

Relevant gates include:

```text
Arduino Uno Build
ESP32 Build
CMP Protocol Tests
Motor reference index
```

A workflow is GREEN only after an actual successful run; documentation-only or unobserved commits do not establish a new firmware GREEN baseline automatically.

### 3. Targeted two-board hardware acceptance

Final physical release confidence remains separate from software cleanup. Required smoke should cover the currently relevant physical boundaries, including where applicable:

- ESP32 -> Arduino JOB delivery and `JOB_ACK`;
- physical START only on Arduino;
- `RUN_STARTED` / `RUN_COMPLETED` propagation and event ACK/retry;
- repeat behavior with no automatic physical START;
- no-run cancel / lost-ACK recovery / `ALL_CLEAR`;
- ESP32 reboot with persisted uncertainty and no auto-resume;
- Hall calibration/telemetry safety;
- manual exact-run exact-spool writeoff after confirmed completed run;
- backup/restore recovery scenarios only on safe/disposable test media where destructive fault injection is required.

Runtime/Serial capture is requested only for an unresolved hardware-only question, not as a routine source-cleanup step.

## Stable safety acceptance criteria

The project must never regress these invariants:

- no automatic physical START;
- no automatic START between repeat runs;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- no automatic winding resume after reboot;
- lost ACK/timeout does not prove Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never performs automatic wire/material writeoff;
- current linked writeoff requires exact `source_session_id + source_run_id + immutable spool_id`;
- cancellation does not erase immutable run/history evidence;
- restore is explicit/operator-only, transactional and fail-closed;
- reboot does not auto-continue restore/apply;
- production data/evidence is never automatically deleted or truncated as a space/recovery shortcut.

## Completion definition

Software cleanup can be marked complete when:

1. remaining owners are classified and no proven duplicate/orphan implementation remains;
2. current high-level/AI/handoff docs match production source paths/contracts;
3. build/test workflow routing matches `cmp-protocol-v1` source ownership;
4. applicable source/static regressions pass;
5. the final cleanup checkpoint records any intentional `REVIEW` items rather than hiding uncertainty.

Hardware acceptance remains an explicit subsequent/parallel release gate and is never inferred from CI.
