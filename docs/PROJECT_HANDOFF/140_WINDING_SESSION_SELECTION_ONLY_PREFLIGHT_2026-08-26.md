# 140 — Winding session selection-only preflight — 2026-08-26

Branch: `cmp-protocol-v1`

## Scope

Reduce duplicate winding-session directory scans without weakening fail-closed persistence recovery semantics.

## Change

`WindingSessionPersistenceIntegrityAudit` previously performed a read-only canonical-name/temp preflight across all three session directories before store `begin()` and then scanned the same directories again for content/metrics.

Audit of the three store begin paths showed:

- `JobSnapshotStore::begin()` only ensures directories; it does not recover or promote temp files.
- `JobStateStore::begin()` only ensures directories; it does not recover or promote temp files.
- `JobSpoolSelectionStore::begin()` may promote one validated recoverable `.json.tmp` selection to the final `.json` path.

Therefore the pre-begin read-only preflight is now retained only for `/data/winding-jobs/spool-selection`. Snapshot and state directories are validated once in their normal content pass, which already rejects non-canonical names/directories and invalid contents.

## Preserved invariants

- selection temp evidence is still observed before any recovery mutation;
- stale/corrupt selection temp remains fail-closed;
- snapshot/state temp or non-canonical entries remain rejected by their content pass;
- immutable snapshot/state/selection cross-link validation is unchanged;
- backup manifest still consumes the same audit metrics;
- no automatic START/resume/writeoff is introduced.

## Commits

```text
1584672e49288334da531235e3bec9f6a691fc7f  source
0e3786c2894cd5b078645ae24ed5ceb3975cb4ea  acceptance contract
```

## Verified CI

```text
ESP32 Build #1606     32981707495 / SUCCESS
CMP Protocol #3644   32981785788 / SUCCESS
```

## Result

Two redundant directory preflight passes are removed from winding-session persistence audit while the only pre-begin mutation-sensitive directory, spool-selection, remains protected.

## Next

Continue bounded search for real duplicate full-file scans. Do not trade away fail-closed integrity, exact provenance, deterministic recovery, or bounded RAM merely for cosmetic API consolidation.
