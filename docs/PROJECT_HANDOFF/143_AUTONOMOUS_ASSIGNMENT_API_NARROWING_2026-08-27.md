# Checkpoint 143 — Autonomous assignment API narrowing (2026-08-27)

## Status

GREEN.

## Source changes

`AutonomousWindingArchive` public API is narrowed so external code uses the checked assignment result path:

- `assignMotorChecked(...)` remains public and preserves `TaskNotFound`, `ArchiveIntegrityFailed`, `StorageUnavailable`, `WriteFailed`, etc.
- `completedTaskExists(...)` moved to private; caller audit found only archive-internal use.
- legacy bool-only `assignMotor(...)` moved to private; caller audit found no external production use after migration to `assignMotorChecked()`.

No archive persistence format, assignment record contents, motor linkage semantics, RUN evidence, or operator behavior changed.

Source commits:

- `5eb76679dce93281f2357793c5a03832b040050f` — hide internal completed-task lookup
- `6f69d0c548303cb9f6920b602cc0d8754deb5d5b` — hide legacy bool assignment API

## Contract

Existing mandatory `Tests/Web/check_arduino_archive_ui.js` checks that:

- `assignMotorChecked()` remains in the public API;
- `CM_AutonomousWindingWeb.cpp` uses the checked result API;
- `completedTaskExists()` and bool `assignMotor()` are absent from the public section and remain internal helpers.

Contract commit:

`38e892edc6a45d9540188516d06c6e41c93abd5d`

## Confirmed CI

- CMP Protocol Tests `#3663`, run `33034665166`: SUCCESS on final source commit `6f69d0c5...`
- ESP32 Build `#1616`, run `33034665123`: build job SUCCESS on final source commit `6f69d0c5...`
- CMP Protocol Tests `#3664`, run `33034707952`: SUCCESS on final contract commit `38e892ed...`

## Related performance audit

`WindingJournal` remains the next high-value growing-file optimization target. Current `save()` and `loadSessionState()` can perform multiple full scans of `/data/winding-runs/events.ndjson` for duplicate/start/active/highest/completed evidence. The next implementation should replace those repeated reads with one authoritative streamed/bounded session-analysis pass, without whole-file buffering or unbounded RAM.

## Safety invariants

Unchanged: no automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact spool/session/run provenance remains mandatory; historical evidence remains immutable; restore remains explicit/operator-only/fail-closed; no automatic production-data deletion or truncation.
