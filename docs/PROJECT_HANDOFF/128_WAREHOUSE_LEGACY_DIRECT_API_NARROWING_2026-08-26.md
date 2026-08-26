# Checkpoint 128 — warehouse legacy direct API narrowing

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Status

**GREEN (software / CI).**

## Change

After public `POST /api/warehouse/write-offs` was permanently disabled with HTTP 410, the old direct Store mutation methods were still exposed publicly:

```text
WarehouseStore::confirmSpoolWriteOff(...)
WarehouseStore::confirmKgFirstWriteOff(...)
```

They remain implemented because deterministic compatibility/recovery and historical transaction semantics must not be deleted blindly, but they are no longer part of the public `WarehouseStore` API.

The declarations were moved behind `private:`.

The managed atomic RUN_WIRE surface remains public:

```text
prepareManagedRunWireWriteOff(...)
applyManagedRunWireSpoolWeight(...)
confirmManagedRunWireWriteOff(...)
```

Exact-run lookup remains public because the atomic coordinator uses it for duplicate protection:

```text
confirmedWriteOffForSourceRun(source_session_id, source_run_id, found)
```

## Why this is safe

ESP32 compilation succeeds with the two old direct methods private, proving current production C++ code has no external caller that depends on those methods being public.

Mandatory host contracts explicitly enforce:

- old direct methods exist only in the private Store surface;
- managed RUN_WIRE methods stay public;
- legacy HTTP POST remains 410-disabled;
- exact-run duplicate protection remains intact;
- low-level deterministic recovery/history implementation remains present.

## Relevant commits

```text
da448296a2d9bb5dcad74983ac322aa479d2327b  refactor(warehouse): narrow legacy direct writeoff API
dfa920aa4c5cc116a6506a0d4eb468d542ace6de  test(warehouse): lock legacy direct methods behind private API
```

## Verified CI

```text
ESP32 Build #1571
run 32962316063
SUCCESS

CMP Protocol Tests #3547
run 32962445538
SUCCESS
68/68 mandatory host steps
```

## Safety invariants preserved

- public legacy writeoff POST remains disabled;
- atomic explicit-operator RUN_WIRE remains the production mutation path;
- low-level recovery code is retained;
- `RUN_COMPLETED` remains non-mutating;
- no automatic START/resume/writeoff was introduced;
- exact spool/session/run/material provenance remains fail-closed.

## NEXT

1. Continue bounded read/report provenance review.
2. Prefer direct immutable transaction fields already persisted by checkpoints 126–127.
3. Do not add redundant full-log scans.
4. Review remaining legacy warehouse types/helpers only when removal/narrowing is compile-proven safe.
5. Continue software optimization/integrity before final two-board hardware E2E.
