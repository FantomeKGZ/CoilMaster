# Checkpoint 145 — WindingJournal boot single-pass validation (2026-08-27)

## Status

GREEN.

## Change

`WindingJournal::begin()` previously reopened `/data/winding-runs/events.ndjson` twice:

1. `validateJournalStructure()` for full record/schema validation;
2. `validateJournalSessionContexts()` for schema-2 session ordering and immutable context consistency.

Checkpoint 145 folds the schema-2 cross-record context checks into `validateJournalStructure()` and removes `validateJournalSessionContexts()`.

The combined boot pass preserves:

- full flat-JSON/schema validation;
- schema version 1 compatibility;
- schema-2 context parsing;
- nondecreasing schema-2 session ids;
- exact `jobId/linked/repairId/motorId` consistency within one schema-2 session;
- RUN_STARTED/RUN_COMPLETED completed-run field validity.

Runtime single-pass analysis from checkpoint 144 is unchanged.

After 144+145, `CM_WindingJournal.cpp` contains exactly two `JournalPath, FILE_READ` sites:

- one combined boot validation pass;
- one runtime session-analysis pass.

## Commits

- header narrowing: `072d754f401577a76c311583e479f0174cd98178`
- source implementation: `bbbc53d96e535204e38db7da0fc79f872dd5a19a`
- mandatory contract: `1760a447ffd216976b844a51adb055a5701f16ff`

## CI evidence

- ESP32 Build `#1620`, run `33035508401`: SUCCESS
- CMP Protocol Tests `#3681`, run `33035532132`: SUCCESS on final contract
- intermediate CMP `#3680` failed only because the previous textual contract still expected three journal read sites before the contract update; it is not final evidence.

## Safety invariants

Unchanged: no automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact spool/session/run provenance remains mandatory; RUN_COMPLETED remains evidence only; historical recovery remains fail-closed; no automatic production-data deletion/truncation.
