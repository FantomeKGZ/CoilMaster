# Checkpoint 166 — Autonomous assignment mutation-time revalidation

Date: **2026-08-29**  
Branch: **`arduino-ru-lcd-experiment`**  
Production remains unchanged at **`cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c`**.

## Result — GREEN

The autonomous motor-assignment flow intentionally has two assignment-ledger proof phases around canonical motor-winding projection:

1. a pre-projection exact assignment lookup, used to avoid duplicate/conflicting canonical projection;
2. a fresh mutation-time assignment-ledger reread immediately before the append.

Those two phases remain separate. Reusing the first proof for the append would weaken the TOCTOU boundary.

Before checkpoint 166, however, the mutation-time pass in `assignMotorChecked()` only validated journal order and derived the next `assignment_id`. If another assignment for the same `session_id + run_id` appeared after the pre-projection proof but before append, that later authoritative pass did not detect the exact retry/conflict.

Checkpoint 166 makes the existing mutation-time assignment-ledger pass supply all append-boundary facts in one bounded streaming scan:

- validates flat JSON records;
- parses and validates every assignment record;
- validates strictly increasing non-zero `assignment_id` ordering;
- finds an exact `session_id + run_id` assignment if one appeared after preflight;
- returns the existing assignment id for an identical `motor_id + role` retry;
- fails closed if the same source run now points to a different motor or role;
- fails closed if duplicate records for the same `session_id + run_id` are present;
- derives the next `assignment_id` from the same validated pass;
- fails closed on id overflow.

The checked mutation path no longer performs a separate `nextAssignmentId()` scan.

## Safety boundary retained

The earlier pre-projection assignment lookup is intentionally retained because canonical projection must know whether the source run is already assigned before it appends motor winding history.

Immediately before assignment append, `assignMotorChecked()` still performs:

- a fresh authoritative `completedTaskExists(sessionId, runId, ...)` reread;
- a fresh full assignment-ledger mutation-time scan.

Therefore canonical projection preflight is **not** reused across the append mutation boundary. The change strengthens race handling while removing only the redundant next-id-only scan.

No whole-file buffering, persistent cache, index or database was introduced.

## Commits

```text
373c62b23d3cf309b70aadd157b921a0bd617671  revalidate autonomous assignment before append
80d1cfbfe61402d7309ac507e476cb9c89fb04d2  lock mutation-time revalidation contract
```

## Verified CI

Runtime commit `373c62b...`:

```text
CMP Protocol Tests #3984  run 33261535647 / SUCCESS
ESP32 Build #1762         run 33261535612 / SUCCESS
Arduino RU LCD #186       run 33261535629 / SUCCESS
```

Contract head `80d1cfb...`:

```text
CMP Protocol Tests #3985  run 33261547591 / SUCCESS
```

## Unchanged invariants

- no automatic physical START/repeat/resume;
- Arduino remains the only SSR owner;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` remains evidence only;
- RUN_WIRE material writeoff remains explicit/manual with exact `spool_id + source_session_id + source_run_id`;
- canonical autonomous projection remains append-only with exact `session_id + run_id + role` provenance;
- assignment retry remains idempotent and conflicting provenance remains fail-closed;
- confirmed history is never automatically deleted/truncated.

## Next

Continue the repeated-growing-journal audit only where a real duplicate scan exists inside the same proof/mutation phase. Preserve preflight-versus-mutation authoritative rereads whenever they form a TOCTOU boundary.