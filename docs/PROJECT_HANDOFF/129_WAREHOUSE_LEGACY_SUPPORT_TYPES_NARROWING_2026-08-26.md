# Checkpoint 129 — warehouse legacy support type narrowing

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Status

**GREEN (software / CI).**

## Change

After checkpoint 128 made the obsolete direct mutation methods private, their request/result support types were still exported at namespace scope:

```text
ConfirmedSpoolWriteOff
SpoolWriteOffResult
```

They are now private nested implementation details of `WarehouseStore`.

The managed production type remains public:

```text
KgFirstWriteOff
```

because atomic RUN_WIRE still passes it through:

```text
prepareManagedRunWireWriteOff(...)
confirmManagedRunWireWriteOff(...)
```

## Read/report audit result

`MaterialRequestMovementStore::appendRequestPageJson()` returns the immutable movement JSON directly. New RUN_WIRE movement rows already contain:

```text
material_request_id
transaction_ref
warehouse_item_id
source_session_id
source_run_id
spool_id
material_class
wire_diameter_hundredths_mm
```

Therefore the existing bounded `/api/material-requests/movements` read surface already exposes direct spool provenance without adding another join or full-log scan. Historic RUN_WIRE rows without `spool_id` remain valid under the checkpoint-127 immutable-selection fallback/integrity rule.

## Why this is safe

- deterministic legacy pending recovery is still implemented;
- legacy journal record shapes are unchanged;
- `appendWriteOffRecord(...)` remains available internally for legacy reboot reconciliation;
- managed RUN_WIRE APIs are unchanged;
- public `POST /api/warehouse/write-offs` remains HTTP 410;
- no persistence migration was introduced.

ESP32 compilation proves no external production C++ caller requires the two hidden support types.

## Relevant commits

```text
b2f7f13f88bf2a5e489999fdf318523fc1fcdf46  refactor(warehouse): hide legacy writeoff support types
071e55923ead09264a801c250e8807a17823eba1  test(warehouse): keep legacy support types private
```

## Verified CI

```text
ESP32 Build #1572
run 32963035385
SUCCESS

CMP Protocol Tests #3551
run 32963113298
SUCCESS
```

## Safety invariants preserved

- `RUN_COMPLETED` remains non-mutating;
- explicit operator RUN_WIRE remains the only production wire mutation path;
- exact spool/session/run/material provenance remains fail-closed;
- recovery/history remain compatible;
- no automatic START/resume/writeoff was introduced;
- no redundant full-log report scan was added.

## NEXT

1. Continue compile-proven narrowing/removal of dead legacy warehouse helpers only where recovery does not depend on them.
2. Keep direct immutable RUN_WIRE movement fields as the preferred bounded report source.
3. Do not duplicate existing cross-log/full-file scans.
4. Continue software optimization/integrity before final two-board hardware E2E.
