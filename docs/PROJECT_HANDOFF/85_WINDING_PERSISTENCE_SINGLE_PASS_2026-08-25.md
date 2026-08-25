# Winding persistence single-pass checkpoint — 2026-08-25

## Context

After the earlier completion/finalization single-pass work, the deep-backup winding persistence audit still performed two full reads of `/data/winding-runs/events.ndjson`:

1. `WindingJournalQuery::validateAll(recordCount)` for full schema validation/counting;
2. `WindingJournalTransitionAudit::validate(storage)` for STARTED/COMPLETED transition integrity.

`WindingJournalTransitionAudit` already performs `WindingJournalQuery::isValidRecord(line)` on every record, so the first pass was redundant.

## Change

- `WindingJournalTransitionAudit` now exposes `validate(storage, recordCount)`.
- The same transition scan now returns the exact validated record count, with a `uint32_t` overflow guard.
- `WindingPersistenceIntegrityAudit::check(storage, recordCount)` now calls that overload directly.
- The separate `WindingJournalQuery::validateAll(...)` pre-pass was removed from this path.

## Preserved invariants

The single pass still performs:

- full winding journal schema validation;
- monotonic session ordering;
- STARTED/COMPLETED transition validation;
- run-id ordering and active-run pairing checks;
- completed-runs sequence validation;
- fail-closed behavior on malformed or unreadable journal data;
- exact record counting for backup metrics.

No change was made to physical START, SSR ownership, Hall calibration, UART protocol, write-off semantics, exact spool provenance, or automatic-resume policy.

## Commits

- `7552312b...` — expose transition audit record-count overload;
- `f8f9b062...` — count records inside the transition pass;
- `beba3528...` — make winding persistence audit single-pass;
- `77d9de5e...` — regression contract;
- `08283225...` — CI gate `Audit winding persistence single-pass contracts`.

## Verification status

Fresh software CI is still required on this batch:

- ESP32 Build;
- CMP Protocol Tests, including `Audit winding persistence single-pass contracts`.

Hardware testing is not required for this storage-only optimization.
