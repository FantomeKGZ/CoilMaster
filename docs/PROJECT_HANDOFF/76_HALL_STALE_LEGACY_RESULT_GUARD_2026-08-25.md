# CoilMaster — Hall stale legacy result guard

Date: **2026-08-25**
Repository: `FantomeKGZ/CoilMaster`
Source of truth: **`cmp-protocol-v1` only**.

## Previous blocks now verified GREEN

User-supplied Actions runs:

```text
32794834301  ESP32 Build
checkout 0099d7241fca6090704c9798f4a6398593fbb52a
build-esp32 SUCCESS

32794834278  CMP Protocol Tests
checkout 0099d7241fca6090704c9798f4a6398593fbb52a
host-tests SUCCESS
Hall calibration safety contracts SUCCESS

32794889210  CMP Protocol Tests
checkout b002d8b71629c9600acc0758ac025ed6589514b0
host-tests SUCCESS

32794909365  CMP Protocol Tests
checkout b374aae424fcfe5df4015647cea8c8f291cc111c
host-tests SUCCESS

32795389705  ESP32 Build
checkout 8280d6066e72033bbbcb10e10415a61212b1c13c
build-esp32 SUCCESS

32795389713  CMP Protocol Tests
checkout 8280d6066e72033bbbcb10e10415a61212b1c13c
host-tests SUCCESS

32795444329  CMP Protocol Tests
checkout 7035dd025a3def712fb775c9513e311bba0e8a3c
host-tests SUCCESS

32795464371  CMP Protocol Tests
checkout 16a75232deeb1677646c8ca70d29953b86e9b684
host-tests SUCCESS
```

This closes software-CI verification for both the stale legacy result drain and the receiver-level rejected-ARM preservation block.

Hardware is not thereby declared GREEN.

## Legacy-result finding

The active Uno completion path remains compact `CAL_DONE`.
ESP32 intentionally retains the legacy `CAL_RESULT` receive fallback for backward compatibility.

A targeted review found one stale-buffer hazard at the Web orchestration boundary:

1. `HardwareControlWeb::handleCalibrationArm()` cleared only its own `m_hasCalibrationResult` flag.
2. `UartEventReceiver::armHallCalibration()` reset compact/raw aggregation but an unread legacy result already buffered inside `HardwareControlClient` could remain available.
3. On the next Web `update()`, that old legacy result could therefore be consumed again, analyzed, and recorded into history during a newly armed calibration cycle.

No active Uno legacy `CAL_RESULT` emitter was reintroduced and no legacy wire-format assumptions were added.

## Stale-result implementation

```text
0099d7241fca6090704c9798f4a6398593fbb52a
fix(hall): clear stale calibration result on arm
```

On a successfully queued new ARM request, the Web layer drains any result already pending in the receiver before exposing the new calibration cycle. This happens synchronously before the next normal `update()` pass.

The guard deliberately does not narrow or remove the legacy parser. A future legacy `CAL_RESULT` arriving after the new cycle begins is still treated according to the existing compatibility path because there is no authoritative historical legacy-valid generation/session contract in the current repository.

Regression:

```text
b002d8b71629c9600acc0758ac025ed6589514b0
test(hall): require stale result drain on arm
```

## Rejected-ARM receiver preservation

A follow-up review found a separate state-loss issue inside `UartEventReceiver::armHallCalibration()`.

Before this fix, the receiver cleared raw aggregation and the compact result **before** calling `HardwareControlClient::armHallCalibration()`.
If the control client rejected the new ARM because another Hall request was already pending, the previous result could be erased even though no new calibration was actually queued.

Implementation:

```text
8280d6066e72033bbbcb10e10415a61212b1c13c
fix(hall): preserve result on rejected arm
```

The receiver now:

1. rejects immediately if the JOB/control lane is busy;
2. asks `HardwareControlClient` to queue the new ARM;
3. returns without altering previous raw/compact state if that queue operation fails;
4. resets raw/compact state only after the new ARM request is successfully queued.

Regression:

```text
7035dd025a3def712fb775c9513e311bba0e8a3c
test(hall): preserve result on rejected arm
```

## Rejected-ARM Web-state preservation

The next review found the same semantic issue one layer higher in `HardwareControlWeb::handleCalibrationArm()`.

Even after the receiver-level fix, a rejected ARM still cleared the Web cache:

- `m_hasCalibrationResult`;
- `m_pendingHistoryMeasurementId`;
- `m_pendingHistoryAbort`.

Therefore an HTTP `409`/busy ARM could make the previous measurement disappear from the Web state even though no new calibration was queued.

Implementation:

```text
ba7d49fe6b2e48f361fb78890830d98b6b814e71
fix(hall): preserve web result on rejected arm
```

The Web layer now drains stale receiver results and clears result/history state only inside the successful `if (accepted)` ARM branch. A rejected ARM leaves the previous Web-visible measurement and pending history state unchanged.

Regression:

```text
ad0015cab7fd1840b0b02f818131cc297c9c3099
test(hall): preserve web state on rejected arm
```

The Hall safety contract now requires the Web result/history clears to remain inside the accepted ARM branch immediately before `queueAccepted(...)`.

## Current CI state

The older stale-result and receiver-level rejected-ARM blocks are verified GREEN by the Actions runs listed above.

The newer Web-state preservation commits `ba7d49fe...` and `ad0015ca...` are **not yet declared GREEN** until fresh Actions runs for these SHAs or descendants are checked.

Do not request an intermediate hardware test for this block.

## Next software review

After fresh CI is GREEN, continue only narrow Hall completion/orchestration review. Do not guess or tighten the historical legacy `CAL_RESULT|VALID` wire contract without authoritative evidence. Final physical hardware acceptance remains deferred until software optimization is complete.
