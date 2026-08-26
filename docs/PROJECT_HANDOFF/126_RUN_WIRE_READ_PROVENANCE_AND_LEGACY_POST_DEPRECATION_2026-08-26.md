# Checkpoint 126 — RUN_WIRE read provenance + legacy writeoff POST deprecation

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Status

**GREEN (software / CI).** Final two-board hardware E2E remains deferred until the software convergence blocks are complete.

## What changed

### 1. New RUN_WIRE Material Request movements expose exact `spool_id`

`NewMaterialRequestMovement` now has an optional `spoolId` field. For `RUN_WIRE`, `MaterialRequestMovementStore::append()` does not trust caller identity: it reloads the immutable `JobSpoolSelection` by `source_session_id`, verifies repair + CU/AL + exact diameter, rejects a conflicting supplied spool, and serializes the authoritative selected `spool_id` into the new immutable movement.

This gives bounded read/report clients direct provenance from `/api/material-requests/movements`:

```text
material_request_id
transaction_ref
warehouse_item_id
source_session_id
source_run_id
spool_id
material_class
wire_diameter_hundredths_mm
quantity_milli_units
unit_cost_minor
cost_amount_minor
currency
created_at
```

Historic RUN_WIRE movement lines without `spool_id` remain readable. Existing integrity code continues to resolve the immutable session selection, so no destructive migration is required.

### 2. Legacy public writeoff mutation is formally disabled

`POST /api/warehouse/write-offs` is now a fail-closed compatibility boundary:

```text
HTTP 410
error = legacy_writeoff_post_disabled
write_performed = false
replacement = /api/material-requests/warehouse
```

`GET /api/warehouse/write-offs` remains available for historical read/coverage/report consumers.

The retired public mutation handler is not present. Low-level warehouse transaction/recovery code remains because atomic RUN_WIRE still emits and reconciles standard physical warehouse evidence and historic recovery must remain deterministic.

### 3. Safety/contract suite migrated to the new boundary

Mandatory host contracts no longer require a reachable legacy mutating Web handler. They now assert:

- public legacy POST always returns `410` and never performs a write;
- production mutation is atomic Material Request `RUN_WIRE` only;
- exact immutable spool/session/run provenance remains required;
- retained low-level warehouse store/recovery remains exact-run protected;
- legacy GET/history remains available;
- no automatic writeoff is introduced.

## Relevant commits

```text
261e76c372e954885ee3975d845e47e608354bbc  movement schema: spoolId
95b025271a799bcf7c175be386c33044c8c4d2b7  derive + persist spool from immutable selection
e4d4e5acd5a08101ae5a6cc29943c228d822bb75  disabled legacy handler cleanup boundary
9f574cf394a2826b71037e29a767e53bfc425124  main RUN_WIRE contract aligned
21f3212d80c61ccaef2225140bfc5c5528577e47  final acceptance contract aligned
```

Additional contract cleanup commits in the same boundary include bridge/release/kg-first/fault assertions migrated from the retired Web mutation path to atomic/store-authoritative safety checks.

## Verified CI evidence

```text
ESP32 Build #1569
run 32960764524
SUCCESS

CMP Protocol Tests #3535
run 32961372178
SUCCESS
```

`CMP #3535` is the first complete mandatory host run after all stale legacy-Web mutation expectations were removed. The ESP32 source tree containing direct RUN_WIRE movement spool provenance and the hard legacy POST boundary compiled successfully in `#1569`.

## Invariants preserved

- `RUN_COMPLETED` is evidence only and never deducts material.
- Physical START remains local-only.
- No auto-resume after reboot.
- Arduino retains SSR ownership.
- ESP32/Web never directly controls SSR.
- Wire issue requires explicit operator confirmation.
- Atomic RUN_WIRE retains one durable high-level recovery owner.
- Exact `material_request_id + warehouse_item_id + source_session_id + source_run_id + spool_id + CU/AL + diameter` provenance is preserved.
- Backup/restore remains fail-closed while RUN_WIRE recovery is unfinished.

## NEXT

1. Extend `RunWireAccountingIntegrityAudit` to validate directly persisted `spool_id` when present while accepting historical movement lines without it.
2. Continue bounded read/report provenance improvements without adding redundant full-log scans.
3. Review whether low-level legacy writeoff APIs can be narrowed further internally without breaking recovery/history compatibility.
4. Continue software/integrity optimization before final two-board hardware E2E.
