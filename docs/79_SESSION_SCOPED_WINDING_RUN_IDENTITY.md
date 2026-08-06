# Session-scoped winding run identity

## Purpose

A winding `run_id` is unique only inside one Arduino `session_id`.
After an Arduino restart a new session may begin with the same run numbers that were used by an older session.

The ESP32 journal therefore identifies an event by the tuple:

```text
(session_id, run_id, event_type)
```

and not by `run_id` alone.

## Behaviour

The journal now:

- detects duplicate `RUN_STARTED` and `RUN_COMPLETED` events only inside the same session;
- matches `RUN_COMPLETED` to `RUN_STARTED` using both `session_id` and `run_id`;
- preserves the rule that `run_id` must increase monotonically inside one session;
- allows a new Arduino session to reuse run numbers from an older session;
- keeps the single-active-run guard isolated per session.

## Example

These two runs are distinct and valid:

```text
session_id=100, run_id=1
session_id=101, run_id=1
```

A repeated event with the same tuple remains a duplicate:

```text
session_id=101, run_id=1, event=RUN_STARTED
session_id=101, run_id=1, event=RUN_STARTED
```

## Safety effect

This prevents an event from a newly restarted Arduino from being rejected merely because an older session used the same `run_id`. It also prevents a completion from being attached to a start belonging to another session.
