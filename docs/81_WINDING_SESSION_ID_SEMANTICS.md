# Winding session identifier semantics

Date: 2026-08-06

Status: DESIGN CONTRACT FOR NEXT IMPLEMENTATION

## Purpose

`session_id` identifies one immutable winding program execution context shared by ESP32 and Arduino.

It is not a boot counter, not a UART connection number, and not a single-coil run number.

## Ownership

ESP32 is the authority that allocates `session_id` because the outgoing job already contains this field:

```cpp
CM::OutgoingWindingJob::sessionId
```

Arduino must not invent or replace the session identifier. After accepting a job, Arduino echoes the accepted `session_id` in all related `RUN_STARTED` and `RUN_COMPLETED` events.

## Allocation rule

For production jobs:

- `session_id` is a non-zero `uint32_t`;
- every newly created immutable job snapshot receives a new identifier;
- identifiers are monotonically increasing on ESP32;
- an identifier is never reused for a different job;
- retrying delivery of the same job keeps the same `session_id`;
- editing a draft before snapshot creation does not allocate a session;
- creating a replacement snapshot allocates a new session.

## Persistence

The last allocated value must be stored durably on ESP32.

Recommended storage responsibilities:

- keep the authoritative counter on microSD together with the job snapshot store;
- update it transactionally before a new job can be delivered;
- recover it at boot from metadata and, if necessary, from the maximum valid stored session identifier;
- never silently reset the counter to 1 when production history exists.

If durable storage is unavailable or inconsistent, ESP32 must not allocate and deliver a new production job. The UI should report a storage/session-allocation fault.

## Restart behavior

### ESP32 restart

ESP32 restores the last allocated identifier and the persisted state of pending or accepted jobs.

A restart must not automatically mark a job completed and must not remotely start SSR.

### Arduino restart

Arduino enters a safe state with SSR off.

An interrupted physical run is not automatically resumed. Recovery requires an explicit state reconciliation and a new physical START after the operator confirms the machine condition.

Arduino may retain the accepted job only if a separate persistence design is implemented and validated. Until then, a restart means the volatile accepted-job state is lost and ESP32 must reconcile or redeliver safely.

## Event identity

A journal event is identified by:

```text
session_id + run_id + event_type
```

Within one session, `run_id` is non-zero and strictly increasing for every new `RUN_STARTED`.

The same numeric `run_id` may exist in another session because the session component keeps identities distinct.

## Full reset and data loss

After intentional factory reset or complete storage replacement, identifier reuse may occur only in a clearly new data domain.

The reset procedure must:

- archive or explicitly discard the old history;
- create a new installation/storage epoch in a future schema;
- prevent old delayed UART frames from being accepted as current events;
- require operator acknowledgement.

Until an epoch field exists, production recovery should prefer preserving the maximum known `session_id` rather than restarting numbering.

## Protocol consequence

Current frames already carry `session_id`, so this contract does not require an immediate frame-format change.

The next implementation stage is a persistent ESP32 session allocator and an immutable job snapshot store linking:

```text
job_id
session_id
repair_id
motor_id
program snapshot
wire selection
creation timestamp
delivery state
execution state
```

## Safety consequence

Possession of a valid `session_id` does not authorize physical motion. Arduino physical START remains mandatory for every coil, and SSR remains under Arduino control.
