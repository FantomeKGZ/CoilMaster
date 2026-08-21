# Checkpoint 59 — Backup session preflight integration

Date: **2026-08-21**  
Branch: **`cmp-protocol-v1`**

## Completed

The duplicate winding-session directory preflight in `CM_BackupExportWeb.cpp` has been removed from the backup manifest stability path.

`WindingSessionPersistenceIntegrityAudit` is now the single authoritative owner of the read-only canonical preflight for:

```text
/data/winding-jobs/snapshots
/data/winding-jobs/state
/data/winding-jobs/spool-selection
```

The backup manifest consumes the audit result directly:

- `directoryPreflightDurationMs` feeds the existing `winding_session_directory_scan_duration_ms` metric;
- `directoryPreflightMeasured` controls whether that metric is reported;
- `DirectoryUnavailable` maps to `session_directory_unavailable`;
- `TemporaryFilePresent` maps to `session_temp_present`;
- `InvalidDirectoryEntry` maps to `session_directory_invalid`;
- content/store validation failures remain `winding_session_persistence_unstable_or_invalid`.

This preserves the existing external backup reason strings while avoiding a second full read-only directory pass before the authoritative persistence audit.

## What was intentionally NOT removed

`scanSessionDirectory()` remains in `CM_BackupExportWeb.cpp` because it is still required by bounded session enumeration:

```text
BackupExportWeb::nextSessionId()
GET /api/backup/sessions
```

Those paths enumerate session IDs and are semantically separate from the backup-manifest stability preflight.

## Regression contract

`Tests/Web/check_final_acceptance_contracts.js` now verifies that:

- the backup manifest consumes `directoryPreflightDurationMs` and `directoryPreflightMeasured`;
- classified directory failures retain the established reason strings;
- the old duplicate manifest call pattern `scanSessionDirectory(storage, directories[i], 0UL, ...)` is not reintroduced.

## Commits

```text
a141f7d5fcf9216a178ca31dbefc6189638f8e22  Consolidate backup session preflight
9d8e3799422fcd6bd38c4a8e89fe6c1f45ad7289  Guard consolidated backup session preflight
```

## Safety invariants unchanged

- physical START remains physical only;
- no automatic physical START between repeat cycles;
- no automatic resume after reboot;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` never performs automatic wire deduction;
- wire deduction remains explicit/manual with exact `source_session_id + source_run_id` provenance;
- backup stability validation remains read-only and fail-closed.

## Verification status

Source and regression-contract integration were reviewed against the current branch contents after the writes.

GitHub workflow configuration still declares `push` on `cmp-protocol-v1`, including `Tests/Web/**` and `firmware/esp32/src/**`, but the connector currently exposes no status checks or workflow runs for HEAD `9d8e3799422fcd6bd38c4a8e89fe6c1f45ad7289`.

Therefore:

```text
CMP Protocol Tests: NOT CONFIRMED
ESP32 build: NOT CONFIRMED
Arduino Uno build: NOT CONFIRMED for this HEAD
GitHub CI: NOT CONFIRMED
```

Do not infer green status from older runs.

## Recovery note

The earlier `00_READ_FIRST.md` entrypoint is stale relative to the actual branch history: project handoff now extends through checkpoints 46–59. Future continuation should prefer the newest checkpoints and current branch code over the older 39–45-only summary.

## Next recovery priority

Continue from current HEAD by checking the remaining build/test regressions around the recent Uno flash/SRAM recovery and then perform a focused source-level audit of changes introduced after the last hardware-accepted baseline. Do not reopen already-closed kg-first/session-preflight work unless a regression is found.
