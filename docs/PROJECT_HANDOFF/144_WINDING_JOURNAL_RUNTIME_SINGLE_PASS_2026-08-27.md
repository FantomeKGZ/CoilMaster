# Checkpoint 144 — WindingJournal runtime single-pass analysis (2026-08-27)

## Status

GREEN.

## Problem

The growing `/data/winding-runs/events.ndjson` journal was reopened repeatedly inside one runtime operation:

- `save(RUN_STARTED)` could perform context + duplicate + active-run + highest-run scans;
- `save(RUN_COMPLETED)` could perform context + duplicate + matching-start + active-run + completed-count scans;
- `loadSessionState()` performed separate active/highest/completed scans.

This made runtime cost scale as several full passes over the same append-only journal.

## Source changes

`WindingJournal` now uses one private streamed `analyzeSession(...)` pass for runtime session evidence. It returns fixed-size state only:

- active run id + presence;
- highest started run id;
- completed-run count;
- exact event replay evidence;
- matching target RUN_STARTED evidence;
- immutable schema-2 context validation when saving.

The old private multi-pass helpers were removed from the class/source:

- `sessionContextMatches`
- `containsRunEvent`
- `hasRunStart`
- `loadSessionCompletedRuns`
- `loadActiveRun`
- `loadSessionHighestRunId`

Runtime effects:

- new RUN_STARTED save: up to ~4 full journal reads -> 1;
- new RUN_COMPLETED save: up to ~5 full journal reads -> 1;
- `loadSessionState`: 3 full journal reads -> 1.

The analyzer remains streamed and bounded; it does not buffer the journal or allocate an unbounded vector.

## Compatibility / fail-closed details

Exact replay remains idempotent. To preserve the previous ordering (`sessionContextMatches` before duplicate detection), once an exact replay is found the analyzer continues to EOF validating the remaining schema/context fields. It intentionally does not start parsing unrelated later event payloads that the old duplicate scan would never have reached.

State-transition errors observed before a later exact replay are remembered instead of immediately returned, preserving the former `Duplicate` result ordering. If no exact replay is found, those errors fail the operation closed.

For non-duplicate operations the same pass validates record fields, schema-2 context, START/COMPLETE pairing, monotonically increasing START run ids, active-run identity and completed-run evidence.

Boot-time `validateJournalStructure()` and `validateJournalSessionContexts()` remain separate. This checkpoint optimizes runtime save/state reads only.

## Commits

- header/runtime analyzer declaration: `8c80cfab6dbe3431587fb4208b87f37936640f94`
- runtime implementation: `baa17db71b3d259d94d22009847c402ae8e6d24c`
- mandatory winding-persistence contract: `b186c085ca58743c77b26da33cbd5d795f126126`

## CI evidence

- ESP32 Build `#1618`, run `33035231152`: SUCCESS
- CMP Protocol Tests `#3673`, run `33035231146`: SUCCESS on final source
- CMP Protocol Tests `#3674`, run `33035275493`: SUCCESS on final contract

The contract also asserts exactly three `JournalPath, FILE_READ` sites remain in `CM_WindingJournal.cpp`: boot structure validation, boot context validation and the single runtime analyzer.

## Safety invariants

Unchanged: no automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact spool/session/run provenance remains mandatory; RUN_COMPLETED remains evidence only; historical evidence remains immutable; restore remains explicit/operator-only/fail-closed; no automatic production-data deletion/truncation.
