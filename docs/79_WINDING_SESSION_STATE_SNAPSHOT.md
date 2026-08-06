# Winding session state snapshot

`CM::WindingJournal` now implements the previously declared method:

```cpp
bool loadSessionState(uint32_t sessionId,
                      WindingSessionState& state) const;
```

The method builds one server-authoritative snapshot from the persisted winding journal.

## Returned state

For the requested Arduino session it returns:

- `sessionId` — requested session identifier;
- `activeRunId` — currently active run, or zero;
- `highestRunId` — highest accepted `RUN_STARTED` identifier;
- `completedRuns` — highest confirmed completed-run counter;
- `activeRunFound` — whether an unfinished run exists;
- `journalConsistent` — true only after all journal scans and invariants succeed.

## Validation

The snapshot is rejected when:

- the journal is not ready;
- `sessionId` is zero;
- the journal cannot be read;
- more than one active run is detected;
- an active run has an invalid identifier;
- the active run identifier is greater than the highest accepted run identifier.

On failure the output structure remains non-authoritative and `journalConsistent` stays false.

## Purpose

This snapshot is intended for the next integration stages:

- restoration of operator state after ESP32 restart;
- web display of the current winding session;
- deciding whether a new program may be queued;
- diagnosing an interrupted run without directly parsing NDJSON in UI code.

The method does not start, resume, cancel, or complete a winding operation. Physical start confirmation and SSR control remain on Arduino.
