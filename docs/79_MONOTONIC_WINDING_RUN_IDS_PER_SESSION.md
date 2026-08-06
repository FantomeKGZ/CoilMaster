# Monotonic winding run identifiers per session

## Purpose

The ESP32 winding journal now requires every new `RUN_STARTED` event to use a `run_id` greater than every previously started run in the same Arduino `session_id`.

## Rule

For one session:

```text
RUN_STARTED run_id=10  -> accepted
RUN_COMPLETED run_id=10
RUN_STARTED run_id=11  -> accepted
RUN_STARTED run_id=10  -> rejected
RUN_STARTED run_id=9   -> rejected
```

A fresh session may start its own run sequence independently.

## Why this is required

The guard prevents delayed, replayed, or stale UART frames from reopening an old run after it has already completed. It also prevents reuse of an identifier that could otherwise make journal history ambiguous.

The existing rules remain active:

- only one run may be active in a session;
- `RUN_COMPLETED` must match the active run;
- completion counters must increase by exactly one;
- duplicate event records are not appended.

## Failure behavior

A non-monotonic start returns:

```cpp
JournalSaveResult::InvalidTransition
```

No journal line is written.

## Storage compatibility

The NDJSON schema is unchanged. The highest prior `run_id` is reconstructed from existing `RUN_STARTED` records in:

```text
/data/winding-runs/events.ndjson
```
