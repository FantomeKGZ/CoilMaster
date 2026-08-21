# Checkpoint 52 — write-off fault-path acceptance

Date: 2026-08-21
Branch: `cmp-protocol-v1`

## Scope closed in this checkpoint

The kg-first/manual wire write-off path was reviewed specifically for failure and reboot behavior. No safety invariant was relaxed and no automatic material deduction was introduced.

### Startup / reboot recovery

`WarehouseStore::begin()` keeps recovery fail-closed and ordered:

1. ensure warehouse directories;
2. recover an interrupted spool-file swap;
3. reconcile a dangling write-off `PENDING`;
4. expose `ready()` only if those stages succeed.

For a dangling `KG_FIRST / UNALLOCATED` transaction, reboot recovery always appends `ABORTED`. Because no spool mutation exists in this mode, recovery must never guess that the material consumption was committed.

For a dangling spool-backed transaction, durable spool state is the proof:

- current spool weight == recorded `weight_before_g` -> append `ABORTED`;
- current spool weight == recorded `weight_after_g` -> append `CONFIRMED`;
- any third/mismatching weight -> recovery fails closed and warehouse readiness is not assumed.

The same rule remains in force for legacy exact-spool transactions.

## Runtime write rejection

The production POST `/api/warehouse/write-offs` rejects the operation before commit when any of the following cannot be proven:

- warehouse/storage readiness;
- repair existence and OPEN lifecycle state;
- immutable job spool-selection provenance;
- exact `source_session_id + source_run_id`;
- authoritative `RUN_COMPLETED` evidence;
- absence of a previous confirmed write-off for the same exact run;
- valid kg-first quantity and, for `UNALLOCATED`, immutable conductor snapshot;
- successful append/mutation/commit transaction.

Store-level `confirmSpoolWriteOff()` / `confirmKgFirstWriteOff()` independently repeat the completed-run and duplicate protection so the HTTP handler is not the sole safety boundary.

## Regression coverage added

New test:

`Tests/Web/check_writeoff_fault_contracts.js`

It statically guards:

- startup recovery ordering and fail-closed readiness;
- `UNALLOCATED PENDING -> ABORTED` after reboot;
- spool-backed `before -> ABORTED` and `after -> CONFIRMED` recovery proof;
- warehouse unavailable / closed repair / missing or corrupt winding history rejection;
- exact-run duplicate rejection;
- UI success only after a successful POST;
- no automatic write-off, auto-resume, or automatic physical START hooks.

CI workflow now invokes this audit on `cmp-protocol-v1`.

## Commits in this block

- `d7441a8d` — initial write-off fault contract test.
- `bdd76ffa` — CI runs the fault contract audit.
- `edd92a15` — align the test with the current kg-first shared UI controller rather than an obsolete wrapper architecture.

## Important observation

The POST handler still has some older error responses that omit the explicit JSON field `write_performed:false` (for example a few early validation/storage failures and some legacy-path errors). They are non-2xx and therefore already fail correctly in the current UI, but HTTP response-shape normalization is a worthwhile follow-up for diagnostics/API consistency. Do not weaken any existing status code or safety check while normalizing it.

## Hardware acceptance still required

Static/repo review cannot prove microSD power-loss timing. Hardware fault injection remains required before calling this production-complete:

1. remove/fail microSD before a manual write-off -> POST must fail and no new confirmed movement may appear;
2. power loss after `PENDING` but before any spool mutation -> reboot must append `ABORTED`;
3. power loss after spool mutation but before `CONFIRMED` -> reboot must append `CONFIRMED` only when spool weight equals the recorded `after` value;
4. corrupt/mismatched spool state -> warehouse must remain fail-closed;
5. repeat the same exact session/run -> duplicate write-off must be rejected;
6. close the repair, then attempt write-off -> reject with no movement;
7. verify both desktop and mobile remain blocked/error-visible for each failed operation.

## Next priority

1. Normalize POST error payloads so every failed manual write-off explicitly returns `write_performed:false` while preserving current HTTP status/error codes.
2. Run/record real-device fault injection for the cases above.
3. Continue NDJSON performance work from checkpoint 51 using measured file sizes/latency; do not introduce automatic cleanup or rotation before provenance-preserving archive semantics are designed.
