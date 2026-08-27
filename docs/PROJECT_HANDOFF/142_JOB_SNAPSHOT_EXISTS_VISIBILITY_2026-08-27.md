# Checkpoint 142 — JobSnapshotStore exists visibility narrowing (2026-08-27)

## Status

Implementation complete; CI evidence is pending because GitHub Actions runs `#3658` and `#1613` are stuck before job creation (`jobs=[]`). These runs are infrastructure-blocked and are not treated as GREEN or RED evidence.

## Source change

`JobSnapshotStore::exists(uint32_t sessionId)` was moved from the public API to the private implementation surface.

Production caller audit found no external use of this convenience helper. Runtime recovery continues to use the authoritative snapshot `load()` / `validateIdentity()` paths.

Source commit:

`7a2639832f1c2c0225fbe0de0a6d817bfc6ba622`

No immutable snapshot format, job/session identity validation, recovery ordering, UART behavior, physical START behavior, SSR ownership, or material write-off semantics changed.

## CI state

- CMP Protocol Tests `#3658`, run `32984867504`: stuck `queued`; GitHub reports no jobs.
- ESP32 Build `#1613`, run `32985063037`: stuck `queued`; GitHub reports no jobs.
- Force-cancel returns HTTP 409 (`Cannot cancel a workflow run that has not been queued yet.`), confirming an Actions orchestration inconsistency rather than a test assertion failure.

Checkpoint 142 must not be promoted to canonical GREEN until a later normal CMP/ESP32 run validates a commit containing `7a263983...`.

## Next bounded software priority

Read-only audit identified a real growing-file hot spot in `WindingJournal`:

- `save()` can scan `events.ndjson` separately for duplicate detection, active run, highest run and completed-run count;
- `loadSessionState()` separately scans the same file for active/highest/completed state;
- future optimization should return bounded session evidence from one authoritative pass rather than adding another parser or unbounded buffering.

Do not start that large journal rewrite while CI infrastructure is unable to execute jobs; keep the design queued and continue only low-risk review/narrowing until compile/host verification is available again.

## Safety invariants

Unchanged: no automatic physical START/repeat START/resume/writeoff; Arduino remains SSR owner; lost ACK never proves idle; exact spool/session/run provenance remains mandatory; restore remains explicit/operator-only/fail-closed; no automatic production-data deletion or truncation.
