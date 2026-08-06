# Composite winding event identity

Date: 2026-08-06

Status: IMPLEMENTED, NOT VERIFIED BY CI API

## Purpose

A winding journal event is identified by the composite key:

```text
session_id + run_id + event_type
```

A numeric `run_id` alone is not globally unique. Different winding sessions may independently contain the same run number.

## Implemented behavior

`CM_WindingJournal` checks duplicates using:

```cpp
containsRunEvent(uint32_t sessionId,
                 uint32_t runId,
                 RemoteEventType type)
```

A start lookup also uses the exact session/run pair:

```cpp
hasRunStart(uint32_t sessionId, uint32_t runId)
```

`RUN_COMPLETED` is accepted only when the same `(session_id, run_id)` has a saved `RUN_STARTED` and is the active run for that session.

## Required results

The following is valid:

```text
session=10, run=1, RUN_STARTED
session=10, run=1, RUN_COMPLETED
session=11, run=1, RUN_STARTED
session=11, run=1, RUN_COMPLETED
```

The second session is not treated as a duplicate of the first.

The following remains a duplicate:

```text
session=10, run=1, RUN_STARTED
session=10, run=1, RUN_STARTED
```

The following remains invalid:

```text
session=10, run=1, RUN_STARTED
session=11, run=1, RUN_COMPLETED
```

because completion does not match the session that owns the start.

## Compatibility

The NDJSON schema remains version 1 and no stored field changes are required.

Existing records already contain both `session_id` and `run_id`, so the stronger identity rule is reconstructed from current data.

## Safety scope

This rule protects journal identity and replay handling. It does not authorize physical start, does not switch SSR, and does not replace the Arduino physical START requirement.

## Verification status

The connected GitHub API did not expose workflow results for the direct commits inspected during this session. Therefore the code must remain marked `NOT VERIFIED` until both workflows are observed green:

```text
ESP32 Build
CMP Protocol Tests
```
