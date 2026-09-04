# Checkpoint 35 — absolute local B return-home — 2026-09-04

## Goal

Make physical keypad `B` an absolute operator exit for a locally entered winding program, including an already-started partial run, so the operator can abandon a wrong winding/wire choice and immediately return to HOME to enter a new program.

Example confirmed requirement:

```text
target 260 turns
40 turns already wound
operator discovers wrong wire
B -> abandon current local program -> HOME
```

## Implementation

Production/source branch: `cmp-protocol-v1`.

Changed `Core/CM_StateMachine.cpp`:

- `StateMachine::returnHome()` now accepts `B` for every local state, including `Winding`, `Paused`, `ManualRun` and `CoilComplete`;
- local program/job state and partial turn count are cleared by `resetToHome()`;
- resulting state is `EnterCoilCount`;
- Arduino output processing observes HOME in the same main loop and removes SSR authority;
- no automatic resume or automatic START is introduced.

Remote jobs remain fail-closed at the core boundary:

- `JobSource::Esp32Web` still rejects ordinary `StateMachine::returnHome()`;
- the existing Arduino `processRemoteOperatorExitKey('B')` remains the explicit remote operator-abort path, where SSR is forced OFF first and the exact remote job cancellation result is sent back to ESP32;
- final remote completion still cannot be silently cleared before exact delivery acknowledgement.

## Regression coverage

Updated `Tests/Protocol/test_return_home_guard.cpp` with explicit coverage for:

- local `260`-turn program abandoned at exactly `40` turns;
- local `Paused` -> HOME;
- local `CoilComplete` -> HOME;
- local `ManualRun` -> HOME;
- remote READY ordinary return-home remains rejected;
- remote active WINDING ordinary return-home remains rejected;
- remote completed run still waits for exact ACK.

## Commits / CI

Implementation:

```text
5516afd594c9ee255623fb482deae0b7f67e5e48
feat(arduino): make B absolute local return home
```

For that implementation commit:

```text
Arduino Uno Build #258
run 33868041146
completed/success
```

The first CMP run for the implementation-only commit failed because the previous regression test still asserted the old local-active-return prohibition. The test was then intentionally updated to the new product contract.

Regression commit:

```text
5d19607483689c1fa95ba5c27f0fbf3cfaf82467
test(arduino): cover absolute local B exit
```

Exact regression CI:

```text
CMP Protocol Tests #4897
run 33868078033
completed/success
```

## Hardware gate

Because this changes physical keypad/machine-state behavior, targeted hardware verification is still required before calling the hardware path verified:

1. enter a local program, e.g. 260 turns;
2. physically START and wind a partial count, e.g. 40;
3. press `B`;
4. confirm motor/SSR turns OFF and LCD returns to HOME/coil-count entry;
5. confirm old target/current turns are cleared;
6. enter a fresh program and confirm it starts only after a new physical START.

Do not infer this physical gate from CI.
