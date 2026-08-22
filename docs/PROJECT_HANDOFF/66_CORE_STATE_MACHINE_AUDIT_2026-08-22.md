# Core state-machine audit checkpoint — 2026-08-22

Branch: `cmp-protocol-v1`

## Scope

This checkpoint continues the strict source audit after `65_ARDUINO_SOURCE_AUDIT_2026-08-22.md` and covers the active `Core/` state/input/job/event ownership reviewed so far:

- `CM_StateMachine.*`
- `CM_InputController.*`
- `CM_KeyMapper.*`
- `CM_NumberInput.*`
- `CM_TurnSource.*`
- `CM_WindingJob.h`
- `CM_WindingEvent.h`
- adjacent UI/state contracts used to prove the transitions.

## Finding 1 — ordinary keypad B could silently erase protected job state — FIXED

Before this audit, `CM_KeyMapper.cpp` mapped keypad `B` to `InputAction::ReturnHome`, and `CM_InputController.cpp` handled that action with an unconditional `resetToHome()`.

That bypassed the stricter production cancellation ownership in `firmware/arduino/src/main.cpp`:

- the explicit remote `JOB_CANCEL` path validates exact accepted READY identity and emits `JOB_CANCEL_ACK`;
- the emergency-clear sequence explicitly refuses `Winding`, `Paused`, `ManualRun`, `CoilComplete`, and `JobComplete`;
- ordinary `B`, however, could previously reset the state machine directly.

Concrete unsafe/mismatched cases included:

- accepted remote `READY` job silently disappearing on Arduino while ESP32 could still regard it as accepted;
- active `Winding` / `Paused` / `CoilComplete` state being erased without the guarded cancellation protocol;
- completed remote run identity being lost before the exact delivery ACK path could clear it authoritatively.

Fix commits:

- `cbdb016dac0d80899cf3359d1ac6cfb53cfc6dd6` — declare guarded `StateMachine::returnHome()`;
- `2349eb29445bd6750062feea3f21be47962ae5be` — implement fail-closed return-home policy;
- `752bf7ea8b6b915c200442971f8f814ec6864507` — route `InputController` ReturnHome through the state-machine guard.

Current policy:

- remote valid job: ordinary return-home is rejected; use the explicit protocol owner;
- `Winding`, `Paused`, `ManualRun`, `CoilComplete`: ordinary return-home rejected;
- local editing and never-started local READY job may return home;
- completed local job may return to menu after its delivery evidence has been independently persisted by the Arduino entrypoint;
- local fault-reset behavior remains available; a remote valid faulted job cannot be silently erased by the ordinary keypad action.

## Finding 2 — manual mode could destroy JOB_COMPLETE ACK state — FIXED

`StateMachine::toggleManual()` previously allowed entry from `MachineState::JobComplete`.

For a final remote run this produced a concrete broken transition:

```text
JobComplete -> ManualRun -> Ready
```

`acknowledgeDeliveredRun()` intentionally requires `MachineState::JobComplete`, exact `currentRunId`, completed status, remote source, and reached repeat target. Therefore a keypad `C` while waiting for the final run ACK could make the later exact ACK impossible to apply.

The JobComplete UI does not advertise manual mode (`A=POVTOR B=MENU`), so this transition had no operator-facing contract supporting it.

Fix:

- `4213a5b54d891a92931ae93b12975dc518034b47` — remove `JobComplete` from manual-mode admission and document the exact-ACK ownership boundary.

`Ready`, `EnterCoilCount`, and `EnterTurns` retain their existing manual-mode behavior; no automatic motor start was added.

## Regression coverage

- `8335351e8ea75a40ca831eec503d86885a812fc5` — new host state-machine guard test;
- `b4e7bd50110e9661888dbfed260be42a6bf35fa6` — register the guard test in CTest;
- `17daf4002ac220d3edccadc2518c2d2e41c1b0b1` — extend coverage so `JobComplete` cannot enter manual mode and exact final ACK remains applicable.

The host test covers:

- remote READY cannot be silently returned home;
- Winding / Paused / CoilComplete cannot be silently erased;
- remote JobComplete remains until exact final ACK;
- JobComplete rejects manual-mode state loss;
- safe local edit / never-started local READY / completed-local menu transitions remain available.

## Other Core review results

- `CM_NumberInput.*` — KEEP; fixed-size `uint16_t` input with explicit digit-count and overflow/range checks; no confirmed defect in this pass.
- `CM_TurnSource.*` — KEEP; simulation source uses wrap-safe unsigned elapsed-time arithmetic and is feature-gated by the Arduino entrypoint; no confirmed defect in this pass.
- `CM_WindingJob.h` — KEEP; fixed-size job model, exact repeat-target semantics for ESP32 jobs, no dynamic allocation.
- `CM_WindingEvent.h` — KEEP; only `RunStarted` / `RunCompleted` production evidence; no material side effects.
- `CM_KeyMapper.*` — KEEP; physical keypad mapping itself is simple; unsafe ReturnHome semantics were in the state/input owner and are fixed above.

## Event ordering check

`firmware/arduino/src/main.cpp` drains `processCoreEvents()` immediately after keypad/external-start processing and again after turn-source processing. Therefore a `RUN_STARTED` published by physical start is consumed before Hall/turn processing can publish `RUN_COMPLETED` in the same outer loop. The single pending-event slot in `StateMachine` is not currently overwritten by the start/completion path under the production loop ordering.

## Safety invariants after fixes

Preserved:

- no automatic physical START;
- no automatic repeat START;
- no reboot auto-resume;
- Arduino remains sole SSR owner;
- ESP32/Web cannot directly energize SSR;
- accepted remote jobs cannot be silently removed by the ordinary menu key;
- final remote completion remains tied to exact delivery ACK;
- `RUN_COMPLETED` does not write off wire/material;
- immutable ESP32 history/writeoff provenance rules remain unchanged.

## Verification state

**PENDING fresh CI/build evidence for this Core batch.**

Do not call commits `cbdb016...` through `17daf400...` GREEN until applicable `CMP Protocol Tests` and Arduino production build evidence are observed. Earlier user-provided GREEN evidence remains valid only for the preceding writeoff-cleanup baseline documented in checkpoints 64/65.

## Next

Continue remaining `Core/` declarations/UI model consistency, then production `Shared/CMP1Text/` and both UART endpoints. The old `Shared/Protocol/` binary host-test layer remains non-production and must not be mistaken for the CMP1 UART source of truth.
