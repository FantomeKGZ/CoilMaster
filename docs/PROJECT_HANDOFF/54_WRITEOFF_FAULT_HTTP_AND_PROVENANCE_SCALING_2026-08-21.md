# Checkpoint 54 — write-off fault semantics and provenance scaling

Date: 2026-08-21
Branch: `cmp-protocol-v1`

## Closed in this checkpoint

### Explicit POST failure semantics

`POST /api/warehouse/write-offs` now returns `write_performed:false` on every JSON failure branch, including legacy validation and commit failures.

This does not change success behavior or material accounting. It only removes ambiguity from rejected requests so UI/test tooling can distinguish a confirmed mutation from a rejected request without inferring from HTTP text alone.

Commits:

```text
875c3e66bd6bf4e937ec9e4e231854af1de68f01
f924bb58134c1ef5d19091d77d800aa9a2c40763
```

The fault-contract test scopes this rule to the POST handler. GET history errors are not required to carry `write_performed` because they cannot create a write.

### Hardware fault-injection plan

A real-device checklist is now saved in:

```text
docs/PROJECT_HANDOFF/53_WRITEOFF_HARDWARE_FAULT_INJECTION_2026-08-21.md
```

It covers:

```text
duplicate exact run
CLOSED repair
microSD unavailable
UNALLOCATED dangling PENDING
SPOOL PENDING before stock mutation
SPOOL PENDING after stock mutation / before CONFIRMED append
ambiguous spool state
damaged winding completion evidence
```

No automatic reboot/fault hook was added to production firmware. Controlled interruption cases must use a disposable test build/media when hardware testing is performed.

### Bounded provenance uniqueness audit

`CM_WarehouseMovementIntegrityAudit.cpp` previously performed one complete `movements.ndjson` scan for every confirmed provenance record. Exact duplicate semantics were correct but I/O grew quadratically with a high constant due to repeated file opens/scans.

The audit now uses a fixed batch of 32 provenance identities:

```text
ProvenanceEntry batch[32]
```

One full candidate scan checks the entire batch. RAM remains bounded while the number of full-file scans is reduced approximately by a factor of 32 for large histories.

Semantics are unchanged:

- legacy session-only confirmed write-off conflicts with every other confirmed write-off for that session;
- exact run-level write-offs conflict only on the same `source_session_id + source_run_id`;
- legacy and KG_FIRST modes share the same uniqueness boundary;
- malformed records remain fail-closed;
- transaction pairing validation still runs before provenance uniqueness validation.

Commits:

```text
3af8e5d73d7172996aa61875a184df00ff90694e
b8c5d89590d62a02e20a6b3eb4a566d29f75c491
```

## Safety invariants unchanged

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web do not drive SSR directly;
- `RUN_COMPLETED` never performs automatic material deduction;
- manual write-off remains exact-run provenance controlled;
- legacy exact-spool history remains readable;
- ambiguous recovery remains fail-closed.

## Verification status

Repository-level contracts were updated, but current ESP32 build and GitHub CI must not be treated as green unless an actual run/result is observed for the current HEAD.

## Next work

1. Observe actual build/CI if available.
2. On hardware, execute checkpoint 53 using disposable repair/spool data.
3. Capture `/api/system/storage` NDJSON sizes and user-visible latency on populated media.
4. Only then choose segmentation/rotation thresholds. Do not introduce destructive compaction or database migration speculatively.
5. If additional repo-only work is needed before hardware testing, prefer eliminating repeated authoritative scans without weakening integrity checks or adding unbounded RAM structures.
