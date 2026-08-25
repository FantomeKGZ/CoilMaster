# 82 — Finalization winding single-pass audit — 2026-08-25

## Scope

Software-only ESP32/storage performance and reliability cleanup on source-of-truth branch `cmp-protocol-v1`.

## Change

`RepairFinalizationGuard::check()` previously performed two full scans of `/data/winding-runs/events.ndjson`:

1. `WindingJournalQuery::validateAll()` for full schema validation;
2. `WindingJournalTransitionAudit::validate(storage)` for STARTED/COMPLETED transition integrity.

After the preceding winding completion single-pass migration, `WindingJournalTransitionAudit::validate()` already performs authoritative full-record schema validation together with transition validation in the same pass. The separate `validateAll()` pre-pass in finalization was therefore redundant.

The finalization guard now calls only `WindingJournalTransitionAudit::validate(storage)` before exact-run write-off coverage.

## Safety preserved

No change to:

- physical START authority;
- SSR ownership;
- reboot/resume behaviour;
- UART/CMP1 wire protocol;
- Hall calibration or telemetry;
- manual write-off requirement;
- exact `source_session_id + source_run_id + spool_id` provenance;
- finalization fail-closed behaviour.

The authoritative transition audit still validates the entire winding journal and rejects malformed schema or invalid STARTED/COMPLETED sequencing before closure can proceed.

## Commits

- `cb03e54a47c0a7e1b23827e51bb01309fb936f77` — `perf(esp32): make finalization winding audit single-pass`
- `ab3efa76e9258d768214b15203f4d478b1f4956d` — `test(esp32): protect finalization winding single-pass audit`
- `92bcc7d5d8773b3e58357743d6332153f43a10d1` — `ci(esp32): audit finalization winding single-pass`

## CI status

Fresh ESP32 Build and CMP Protocol Tests for this batch are pending. Do not mark this block GREEN until verified on these commits or a descendant.

Expected CMP step:

`Audit finalization winding single-pass contracts`

Hardware acceptance is intentionally deferred until the software review sequence is complete.
