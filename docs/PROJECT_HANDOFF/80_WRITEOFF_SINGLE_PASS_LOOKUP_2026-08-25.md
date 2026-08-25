# CoilMaster — write-off exact-run lookup scan reduction

Date: **2026-08-25**
Repository: `FantomeKGZ/CoilMaster`
Source of truth: **`cmp-protocol-v1` only**.

## Previous Uno resource guard — software CI GREEN

Operator-supplied verification:

```text
32802199135  Arduino Uno Build @ ccfe0f2f... SUCCESS
32802252502  CMP Protocol Tests @ b567a509... SUCCESS
```

The Arduino Uno workflow resource guard is active and passed:

```text
minimum RAM headroom   512 bytes
minimum Flash headroom 512 bytes
```

Uno SRAM micro-optimization is closed. Current measured implementation baseline remains approximately RAM 1205/2048 and Flash 31460/32256; avoid flash-expensive Uno changes without a concrete defect.

## Current ESP32/storage reliability batch

Review of manual write-off duplicate detection found an unnecessary complete `movements.ndjson` pass.

Before this batch, `WarehouseStore::confirmedWriteOffForSourceRun()` performed:

1. authoritative `WarehouseMovementIntegrityAudit::check()`;
2. then opened `movements.ndjson` again and scanned every record only to resolve whether the requested `source_session_id + source_run_id` was already CONFIRMED.

The authoritative audit already parses every transaction during its primary validation pass. The exact-run lookup is now resolved during that same pass.

Implementation:

```text
657a91b184e7c3c7d961d202bd1f053fb7d0854c
perf(esp32): resolve writeoff run during integrity audit

192b9e7b27fc46138e8deee4250a0ce96d7ab85d
perf(esp32): resolve writeoff run during integrity audit

61eaef125d87bf3d876fad34f4cb164c38d32a1f
perf(esp32): reuse movement audit for writeoff lookup
```

New audit API:

```text
WarehouseMovementIntegrityAudit::checkSourceRun(
    storage, sourceSessionId, sourceRunId, confirmed)
```

Safety/integrity semantics are unchanged:

- PENDING -> CONFIRMED|ABORTED pairing is still validated;
- malformed records remain fail-closed;
- dangling PENDING remains invalid;
- confirmed provenance uniqueness still runs;
- duplicate protection remains exact `source_session_id + source_run_id`;
- store-level duplicate/completion protection remains authoritative;
- no automatic material deduction was introduced.

Only the redundant post-audit full-file lookup pass was removed.

Regression:

```text
1560f4ebcf5cb019a8250f4be783d1f1f678daa0
test(esp32): protect single-pass writeoff lookup
```

The existing write-off fault contract now requires the audit-level exact-run API and rejects restoration of direct `FILE_READ` / `readStringUntil` scanning in `CM_WarehouseWriteOffLookup.cpp`.

## Verification state

Fresh verification is pending for this batch:

- ESP32 Build on `61eaef12...` or descendant;
- CMP Protocol Tests on `1560f4eb...` or descendant, especially `Audit write-off fault contracts`.

Do not call this batch GREEN until those Actions are observed.

## Safety boundary unchanged

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly control SSR;
- `RUN_COMPLETED` never automatically deducts material;
- write-off remains explicit/manual and bound to exact session + run + immutable spool.

## Next software step

After fresh CI, continue bounded ESP32/storage performance review of repeated authoritative NDJSON scans. Do not introduce destructive compaction, automatic truncation, or database migration without measured need.
