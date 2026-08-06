# Complete winding journal record writes

The winding event journal now treats a record as saved only when the storage layer reports that the complete NDJSON line was written.

## Previous behaviour

`WindingJournal::appendRecord()` used `File::printf()` and considered any positive return value successful. A short or partial write could therefore be reported as saved even though the last line in `/data/winding-runs/events.ndjson` was incomplete.

An incomplete final line could make later transition checks unreliable because the journal is the authoritative source for:

- duplicate event detection;
- one active run per Arduino session;
- monotonic `run_id` validation;
- `RUN_STARTED` before `RUN_COMPLETED`;
- sequential `completed_runs` validation.

## New behaviour

The complete JSON record is assembled first and then written with `File::print()`.

Success requires:

```text
written_bytes == record_length
```

If fewer bytes are reported, `appendRecord()` returns `false` and `save()` returns:

```text
JournalSaveResult::WriteFailed
```

The event is not acknowledged as durably saved by the journal layer.

## Compatibility

The NDJSON schema and file path are unchanged:

```text
/data/winding-runs/events.ndjson
```

No public C++ signatures or UART frames were changed.
