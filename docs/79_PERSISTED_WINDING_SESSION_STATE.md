# Persisted winding session state

## Purpose

The ESP32 winding journal can now reconstruct the durable state of one Arduino session without relying on volatile RAM.

The new API is:

```cpp
WindingSessionState state;
const bool loaded = journal.loadSessionState(sessionId, state);
```

## Returned state

`WindingSessionState` contains:

- `sessionId` — requested Arduino session;
- `activeRunId` — currently unfinished run, or zero;
- `highestRunId` — highest accepted `RUN_STARTED` identifier in the session;
- `completedRuns` — highest durable completion counter;
- `activeRunFound` — whether an unfinished run exists;
- `journalConsistent` — whether the reconstructed state passed basic invariants.

## Safety rules

The lookup fails when:

- the journal is not ready;
- `sessionId` is zero;
- the journal cannot be read;
- active-run reconstruction detects overlapping runs;
- the reconstructed counters violate basic consistency rules.

The method is read-only. It does not start a motor, acknowledge an Arduino event, alter SSR state, or modify the journal.

## Intended use

This state will support the next integration steps:

- restoring the operator screen after ESP32 reboot;
- preventing a second program from being issued while a durable run is active;
- showing the last completed coil count;
- reconciling UART state with the SD-card journal before accepting new work.
