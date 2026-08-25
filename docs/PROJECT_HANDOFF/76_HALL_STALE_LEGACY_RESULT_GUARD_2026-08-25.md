# CoilMaster — Hall stale legacy result guard

Date: **2026-08-25**
Repository: `FantomeKGZ/CoilMaster`
Source of truth: **`cmp-protocol-v1` only**.

## Previous block now verified GREEN

User-supplied Actions run:

```text
32793788974  CMP Protocol Tests
checkout e7be56811f1ff804af9a4f380326bc26aa3be864
host-tests SUCCESS
Hall calibration safety contracts SUCCESS
```

This closes the software-CI verification for:

- CAL_APPLIED exact measurement id + sent proposal correlation;
- CAL_STATE ARM/ABORT/GET sent-request correlation;
- stale state protection added in the preceding Hall request/reply review.

Hardware is not thereby declared GREEN.

## New legacy-result finding

The active Uno completion path remains compact `CAL_DONE`.
ESP32 intentionally retains the legacy `CAL_RESULT` receive fallback for backward compatibility.

A targeted review found one stale-buffer hazard at the Web orchestration boundary:

1. `HardwareControlWeb::handleCalibrationArm()` cleared only its own `m_hasCalibrationResult` flag.
2. `UartEventReceiver::armHallCalibration()` reset compact/raw aggregation but an unread legacy result already buffered inside `HardwareControlClient` could remain available.
3. On the next Web `update()`, that old legacy result could therefore be consumed again, analyzed, and recorded into history during a newly armed calibration cycle.

No active Uno legacy `CAL_RESULT` emitter was reintroduced and no legacy wire-format assumptions were added.

## Implementation

```text
0099d7241fca6090704c9798f4a6398593fbb52a
fix(hall): clear stale calibration result on arm
```

On a successfully queued new ARM request, the Web layer now drains any result already pending in the receiver before exposing the new calibration cycle. This happens synchronously before the next normal `update()` pass.

The guard deliberately does not narrow or remove the legacy parser. A future legacy `CAL_RESULT` arriving after the new cycle begins is still treated according to the existing compatibility path because there is no authoritative historical legacy-valid generation/session contract in the current repository.

## Regression contract

```text
b002d8b71629c9600acc0758ac025ed6589514b0
test(hall): require stale result drain on arm
```

`Tests/Web/check_hall_calibration_contracts.js` now requires:

- accepted `armHallCalibration()` before the drain;
- drain through `takeHallCalibrationResult(...)`;
- Web result availability cleared after the drain.

## CI state

The new stale-result implementation/regression commits are **not yet declared GREEN** until fresh Actions runs for `0099d724...`, `b002d8b7...` or descendants are checked.

Do not request an intermediate hardware test for this block.

## Next software review

After fresh CI is GREEN, continue only narrow Hall completion/orchestration review. Do not guess or tighten the historical legacy `CAL_RESULT|VALID` wire contract without authoritative evidence. Final physical hardware acceptance remains deferred until software optimization is complete.
