# Winding journal startup structure validation

The ESP32 winding journal now validates every persisted NDJSON record during `WindingJournal::begin()` before the journal becomes ready.

A record is accepted only when it contains:

- a complete JSON object on one line;
- `schema_version` equal to `1`;
- non-zero `run_id` and `session_id`;
- `completed_runs` within the `uint16_t` range;
- an `uptime_ms` field;
- exactly one supported event marker: `RUN_STARTED` or `RUN_COMPLETED`;
- `completed_runs == 0` for `RUN_STARTED`;
- `completed_runs > 0` for `RUN_COMPLETED`.

If a malformed, truncated, unsupported, or structurally inconsistent record is found, `begin()` returns `false` and the journal stays unavailable. New winding events are therefore not appended to a history whose state cannot be trusted.

This is a fail-closed integrity rule. It does not automatically delete or rewrite the damaged journal file, preserving it for diagnosis and controlled recovery.
