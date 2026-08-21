# Checkpoint 58 — Backup session preflight consolidation

Date: 2026-08-21
Branch: `cmp-protocol-v1`

## Completed in this checkpoint

`WindingSessionPersistenceIntegrityAudit` now owns a classified, measured, read-only directory preflight for:

- `/data/winding-jobs/snapshots`;
- `/data/winding-jobs/state`;
- `/data/winding-jobs/spool-selection`.

The preflight is completed before any store `begin()` call. This remains necessary because a store begin path may recover a temporary file, while deep backup must fail closed instead of mutating ambiguous persisted state during validation.

New failure classification:

- `DirectoryUnavailable`;
- `TemporaryFilePresent`;
- `InvalidDirectoryEntry`;
- `ContentInvalid`;
- `None`.

New metrics:

- `directoryPreflightMeasured`;
- `directoryPreflightDurationMs`.

Canonical `session-<id>.tmp` is explicitly classified as `TemporaryFilePresent`. Other non-canonical entries remain invalid.

`Tests/Web/check_final_acceptance_contracts.js` now protects the ordering invariant that read-only preflight must occur before `JobSnapshotStore` / spool-selection store begin paths.

## Important remaining integration

`CM_BackupExportWeb.cpp` currently still performs its own session-directory scan before calling `WindingSessionPersistenceIntegrityAudit`.

Therefore the duplicate directory scan is **not yet considered removed**. The session audit now exposes exactly the failure/timing information needed to remove that outer scan while preserving existing manifest reasons:

- `session_directory_unavailable`;
- `session_temp_present`;
- `session_directory_invalid`;
- generic `winding_session_persistence_unstable_or_invalid` for content failures.

The next patch should wire `SnapshotAuditMetrics::windingSessionDirectoryScanTiming` from `WindingSessionPersistenceAuditMetrics::directoryPreflightDurationMs` and map the classified failure to the existing reason strings, then delete only the redundant manifest scan.

## Safety invariants unchanged

- physical START remains physical only;
- no automatic resume after reboot;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` never performs wire deduction;
- wire deduction remains manual and exact-run provenance remains authoritative;
- backup remains read-only/fail-closed during stability validation.

## Verification status

Source/static-contract reviewed only for this code batch.

ESP32 build: **NOT CONFIRMED**.
GitHub CI: **NOT CONFIRMED unless an actual workflow/status result is visible**.
