# Checkpoint 143 — Autonomous assignment API narrowing (2026-08-27)

## Status

Implementation and contract update complete. CI validation is pending because GitHub Actions is currently failing to create jobs for new workflow runs in this repository. Infrastructure-blocked runs are not treated as GREEN or RED evidence.

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

Existing mandatory `Tests/Web/check_arduino_archive_ui.js` now checks that:

- `assignMotorChecked()` remains in the public API;
- `CM_AutonomousWindingWeb.cpp` uses the checked result API;
- `completedTaskExists()` and bool `assignMotor()` are absent from the public section and remain internal helpers.

Contract commit:

`38e892edc6a45d9540188516d06c6e41c93abd5d`

## Related performance audit

A separate read-only audit identified `WindingJournal` as the next high-value growing-file optimization target. Current `save()` and `loadSessionState()` can perform multiple full scans of `/data/winding-runs/events.ndjson` for duplicate/start/active/highest/completed evidence. Future work should replace these with one authoritative streamed/bounded session-analysis pass, without whole-file buffering or unbounded RAM.

Large journal rewrites should wait until ESP32/CMP CI can execute jobs again.

## Safety invariants

Unchanged: no automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact spool/session/run provenance remains mandatory; historical evidence remains immutable; restore remains explicit/operator-only/fail-closed; no automatic production-data deletion or truncation.
