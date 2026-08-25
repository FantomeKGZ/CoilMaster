# Hall sent-proposal correlation CI fix — 2026-08-25

Repository: `FantomeKGZ/CoilMaster`  
Source of truth: `cmp-protocol-v1` only.

## Verified Actions

- `32793319149` — ESP32 Build — checkout `aa0d4d91bc3f2f9411197d9c63cdd9ef8ce72b19` — **SUCCESS**.
- `32793319169` — CMP Protocol Tests — same checkout — **FAILURE** only in `Audit Hall calibration safety contracts`.
- CTest itself passed 4/4 and all other listed host audits passed.

## Failure cause

The firmware change was not the cause of the failure. `Tests/Web/check_hall_calibration_contracts.js` still required the older textual shape of the stale `WAITING_APPLY_CONFIRM` guard. The implementation had moved sent-state correlation into the proposal keepalive/staged-request condition, so the static string assertion became stale.

Failing assertion expected:

```text
m_requestType == RequestType::StageCalibrationProposal &&
!m_waitingReply &&
parsed.state == HallCalibrationRemoteState::WaitingApplyConfirm
```

Current safe implementation instead requires `m_waitingReply` before treating a staged proposal as already sent/active.

## Fix

```text
2f5625636cf89b90f9582adc33d4c97889015422
test(hall): align sent proposal correlation contract
```

The Hall contract now checks the current sent-state semantics and also explicitly requires the `CAL_APPLIED` completion guard:

```text
RequestType::StageCalibrationProposal
+ m_waitingReply
+ exact pending measurement_id
```

No firmware safety guard was removed or weakened.

## Current status

- Firmware commit `aa0d4d91...`: ESP32 build verified GREEN.
- Host-tests for `aa0d4d91...`: red only because of stale contract text.
- Contract-fix commit `2f562563...`: fresh CI pending; do not call this block fully GREEN until host-tests on this commit or a descendant pass.
- Hardware remains untested in this optimization phase; do not request intermediate physical testing.
