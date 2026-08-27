# Checkpoint 142 — JobSnapshotStore exists visibility narrowing (2026-08-27)

## Status

GREEN. The original `#3658/#1613` runs were stuck in GitHub Actions before job creation, but later successful CMP/ESP32 runs validated commits containing this source change.

## Source change

`JobSnapshotStore::exists(uint32_t sessionId)` was moved from the public API to the private implementation surface.

Production caller audit found no external use of this convenience helper. Runtime recovery continues to use the authoritative snapshot `load()` / `validateIdentity()` paths.

Source commit:

`7a2639832f1c2c0225fbe0de0a6d817bfc6ba622`

No immutable snapshot format, job/session identity validation, recovery ordering, UART behavior, physical START behavior, SSR ownership, or material write-off semantics changed.

## CI evidence

Original blocked runs:

- CMP Protocol Tests `#3658`, run `32984867504`: stuck before job creation
- ESP32 Build `#1613`, run `32985063037`: stuck before job creation
- force-cancel returned HTTP 409; these were infrastructure-blocked and are not used as RED evidence

Later revalidation containing checkpoint 142:

- CMP Protocol Tests `#3663`, run `33034665166`: SUCCESS
- ESP32 Build `#1616`, run `33034665123`: build job SUCCESS
- CMP Protocol Tests `#3664`, run `33034707952`: SUCCESS

Checkpoint 142 is therefore part of the canonical GREEN foundation.

## Next bounded software priority

Read-only audit identified a real growing-file hot spot in `WindingJournal`:

- `save()` can scan `events.ndjson` separately for duplicate detection, active run, highest run and completed-run count;
- `loadSessionState()` separately scans the same file for active/highest/completed state;
- next optimization should return bounded session evidence from one authoritative streamed pass rather than adding another parser or unbounded buffering.

## Safety invariants

Unchanged: no automatic physical START/repeat START/resume/writeoff; Arduino remains SSR owner; lost ACK never proves idle; exact spool/session/run provenance remains mandatory; restore remains explicit/operator-only/fail-closed; no automatic production-data deletion or truncation.
