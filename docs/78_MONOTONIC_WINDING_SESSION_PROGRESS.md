# Monotonic winding session progress

## Purpose

The ESP32 winding journal now validates the cumulative `completed_runs` counter for every Arduino session.

## Rules

- `RUN_STARTED` must contain `completed_runs = 0`.
- `RUN_COMPLETED` must have a previously saved `RUN_STARTED` with the same `run_id` and `session_id`.
- The first accepted completion in a session must contain `completed_runs = 1`.
- Every later completion must increase the saved session maximum by exactly one.
- Regressions, repeated counters, skipped counters and overflow are rejected as `JournalSaveResult::InvalidTransition`.

Examples:

- saved maximum `3`, incoming completion `4`: accepted;
- saved maximum `3`, incoming completion `3`: rejected;
- saved maximum `3`, incoming completion `5`: rejected;
- no previous completion, incoming completion `2`: rejected.

## Persistence

The previous session maximum is reconstructed from `/data/winding-runs/events.ndjson`. The rule therefore remains effective after an ESP32 restart.

## Safety scope

This validation does not start the machine and does not control the SSR. Physical start confirmation on Arduino remains mandatory for each coil.
