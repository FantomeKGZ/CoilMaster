# Checkpoint 127 — persisted RUN_WIRE spool integrity

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Status

**GREEN (software / CI).** Final two-board hardware E2E remains deferred until the remaining software/report integrity work is complete.

## Scope

Checkpoint 126 made exact `spool_id` directly readable from new immutable RUN_WIRE Material Request movements while keeping historical lines backward-compatible.

Checkpoint 127 closes the integrity side of that change without adding another log scan.

## Implementation

`CM_RunWireAccountingIntegrityAudit.cpp` now records two distinct spool identities while reading the existing RUN_WIRE movement batch:

```text
hasPersistedSpoolId
persistedSpoolId
spoolId  // authoritative immutable selection result
```

During the existing Material Request movement pass:

- `spool_id` is optional, preserving historical movement records;
- when present it must parse as a non-zero unsigned ID;
- no second Material Request movement scan is opened.

During the existing immutable session-selection resolution:

```text
if persisted spool_id exists
    persisted spool_id must equal JobSpoolSelection.spoolId
else
    historical record continues using JobSpoolSelection as authority
```

Only after this check does the audit accept bridge, MaterialLedger and warehouse CONFIRMED evidence.

A mismatch therefore fails closed before downstream cross-log evidence is trusted.

## Boundedness

The existing design remains unchanged:

```text
ReferenceBatchSize = 16
one Material Request movement log pass
bounded immutable spool/bridge resolution
one Ledger evidence pass per batch
one warehouse evidence pass per batch
no new full-log pass for persisted spool_id
```

## Relevant commits

```text
6965bd716ac9f4d3970bc750a8e8933b7b6fffd0  audit(run-wire): validate persisted spool provenance
38883ae01493622d1bc98fc179fb9d9eb571ddcf  test(run-wire): require optional persisted spool audit
```

## Verified CI evidence

```text
ESP32 Build #1570
run 32961925117
SUCCESS

CMP Protocol Tests #3541
run 32961999553
SUCCESS
68/68 mandatory host steps passed
```

## Safety properties preserved

- Historical RUN_WIRE movements without `spool_id` are not invalidated.
- New directly persisted `spool_id` cannot disagree with immutable session selection.
- `RUN_COMPLETED` remains non-mutating.
- Material deduction remains explicit operator action only.
- Public legacy warehouse writeoff POST remains HTTP 410-disabled.
- Exact session/run/spool/bridge/material identity remains fail-closed.
- No automatic START/resume/writeoff behavior was added.

## NEXT

1. Review bounded read/report surfaces and expose transaction provenance only where it avoids reconstruction or ambiguous joins.
2. Prefer existing cross-log batches/read APIs; do not add redundant full-log scans.
3. Review whether retained low-level legacy writeoff APIs can be narrowed to internal managed/recovery use without breaking deterministic recovery or history.
4. Continue software/integrity optimization before final two-board hardware E2E.
