# CoilMaster — winding completion single-pass audit

Date: **2026-08-25**
Repository: `FantomeKGZ/CoilMaster`
Source of truth: **`cmp-protocol-v1` only**.

## Context

The exact-run completion gate used by manual write-off called `WindingSessionCompletionAudit::check()`. Before this checkpoint, one completion check performed two complete reads of `/data/winding-runs/events.ndjson`:

1. `WindingJournalQuery::validateAll()` validated the complete schema of every record;
2. `WindingJournalTransitionAudit::validate(...)` reopened the same journal and validated STARTED/COMPLETED ordering while resolving completion evidence.

The duplicate I/O becomes increasingly expensive as the append-only journal grows. Both Web preflight and the authoritative store-level write-off guard call the completion audit, so the cost is visible on the manual write-off path.

## Change

`WindingJournalQuery` now exposes the existing authoritative per-record schema check as:

```cpp
static bool isValidRecord(const String& line);
```

`validateAll()` reuses that exact validator, preserving its existing public full-file behavior.

`WindingJournalTransitionAudit::validateInternal()` now calls `WindingJournalQuery::isValidRecord(line)` for every journal line during the transition scan. It therefore validates the full schema and the transition sequence in the same file pass.

`WindingSessionCompletionAudit::check()` no longer performs a separate `validateAll()` pre-pass. It delegates to the schema-aware transition audit and retains the same result mapping:

- storage unavailable -> `StorageUnavailable`;
- malformed schema or invalid transition sequence -> `IntegrityFailed`;
- exact completed run -> `Completed`;
- otherwise -> `NotCompleted`.

## Commits

```text
77f138b655f51d2c6a41e5902bbfdf611d12fd50
perf(esp32): expose winding record validation

efe8475b4421b32a21be8bfd9b856abd351252ec
perf(esp32): reuse winding schema validator

71ba87b3eb640c2ef6b178f4e5ceda98ff86226f
perf(esp32): fold winding schema into transition audit

c51244b9a32c6f2a8757a572e3a18746bfba04a2
perf(esp32): make completion audit single-pass

d009a0a1cd93cb7ad26290e1d1ad9f607f2f5ef9
test(esp32): preserve completion audit migration marker

15e307615ab92671e1e955795fdfcc1d9b8b3498
test(esp32): protect single-pass winding completion audit

11b976cae2c17eb99407b1a14dd27bad71ce8310
ci(esp32): audit single-pass winding completion
```

## Safety semantics unchanged

This optimization does **not** change:

- physical START authority;
- SSR ownership;
- reboot/auto-resume behavior;
- RUN_COMPLETED material policy;
- exact `source_session_id + source_run_id` completion requirement;
- immutable spool provenance;
- manual write-off requirement;
- fail-closed behavior on malformed journal data.

No automatic write-off path was introduced.

## Expected performance effect

For one `WindingSessionCompletionAudit::check()` call, winding journal file reads are reduced from two complete passes to one complete pass. Schema fields are still parsed and validated; only duplicate storage I/O is removed.

The manual Web write-off path still intentionally performs a preflight completion check and the store repeats the authoritative safety check. This checkpoint does not weaken that caller-level defense-in-depth.

## Verification state

Fresh CI for this batch is **pending**. Do not mark this checkpoint GREEN until observed:

- ESP32 Build on `c51244b9...` or descendant containing the full implementation;
- CMP Protocol Tests on `11b976ca...` or descendant;
- `Audit kg-first material contracts` SUCCESS;
- `Audit winding completion single-pass contracts` SUCCESS;
- `Audit write-off fault contracts` SUCCESS.

Final two-board hardware acceptance remains deferred until software optimization/review is complete.
