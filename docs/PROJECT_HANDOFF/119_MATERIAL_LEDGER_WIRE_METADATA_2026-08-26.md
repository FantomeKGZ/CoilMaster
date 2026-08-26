# 119 — MaterialLedger Wire Metadata — 2026-08-26

Status: GREEN

## Purpose
Add authoritative structured wire identity to the existing generic MaterialLedger without breaking legacy/generic catalog records, stock units, costing, or the current exact-spool production path.

## Contract
`/data/materials/materials.ndjson` remains the authoritative generic material catalog.

Optional wire metadata pair:

```text
wire_type = CU | AL
diameter_hundredths_mm = 1..65535
```

Rules:
- legacy/generic material records may omit both fields and remain valid;
- if either field is present, both are required;
- wire metadata is valid only for `unit=GRAM`;
- existing `stock_quantity_milli`, price, currency and cost formula semantics are unchanged.

## Implemented
- `NewMaterial` and `MaterialItemState` now carry explicit optional wire metadata.
- `POST /api/materials` accepts the pair only when valid and explicit.
- `MaterialLedger::addMaterial()` fail-closes invalid/inconsistent metadata and serializes valid wire metadata.
- active material state lookup returns structured wire metadata.
- material swap/recovery validation and material list parsing validate the optional pair while preserving old records.
- `SpoolMaterialBridgeIntegrityAudit` now requires exact agreement between bridge and MaterialLedger item:
  - unit `GRAM`;
  - exact `CU|AL`;
  - exact diameter.
- permanent regression `Tests/Web/check_material_wire_metadata.js` is wired into CMP Protocol Tests.

## Safety state
- no spool bridge HTTP/runtime writer is exposed by this block;
- existing exact-spool writeoff/finalization remains authoritative;
- no automatic warehouse mutation is introduced;
- `RUN_COMPLETED` still never deducts material;
- existing generic/non-wire materials remain backward-compatible.

## Defect found and fixed during verification
Initial ESP32 compile exposed a missing closing namespace brace in `CM_MaterialLedgerSwap.cpp`. The fix was limited to the missing namespace closure.

## Verification
- CMP Protocol Tests `#3443`, run `32941574082`: SUCCESS, including `Audit MaterialLedger wire metadata`.
- ESP32 Build `#1538`, run `32941574080`: SUCCESS.

## Next
Add an explicit operator-only spool-material bridge creation flow. Creation must fail closed unless:
- exact active physical spool exists;
- exact active MaterialLedger item exists;
- MaterialLedger unit is `GRAM`;
- physical spool, catalog item and requested bridge agree on `CU|AL` and exact diameter;
- the physical spool is not already bridged.

Bridge creation must append identity evidence only; it must not mutate stock, perform writeoff, start hardware, or weaken existing exact-spool finalization/writeoff requirements.
