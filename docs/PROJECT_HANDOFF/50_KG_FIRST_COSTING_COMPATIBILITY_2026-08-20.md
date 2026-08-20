# Checkpoint 50 — kg-first costing and legacy compatibility

Date: 2026-08-20
Branch: `cmp-protocol-v1`

## Fixed consumer mismatch

`RepairCosting::load()` still contained a second legacy spool-only parser for `/data/warehouse/movements.ndjson`. That parser required `spool_id`, `weight_before_g`, and `weight_after_g` on every record, so a valid `KG_FIRST / UNALLOCATED` movement could pass the authoritative warehouse integrity audit and then make repair costing fail closed.

This duplicate parser has been removed from the wire-cost path.

Costing now:

1. runs `WarehouseMovementIntegrityAudit::check()` as the authoritative whole-journal transaction/provenance check;
2. parses each movement through `WarehouseWriteOffRecordCodec`;
3. consumes only `CONFIRMED` records for the requested repair;
4. calculates wire value from exact `massGrams * pricePerKgMinor` with the existing rounding rule;
5. aggregates CU / AL / legacy UNKNOWN material without requiring a spool for kg-first records.

This keeps costing aligned with history, finalization, recovery, and warehouse integrity instead of maintaining a drifting second movement schema.

## Reports

The closed-repair reports UI does not parse warehouse movements directly. It reads `/api/repairs/costing`, so the repaired costing consumer is the financial source used by reports as well.

## Legacy UNKNOWN compatibility

The shared write-off codec was also corrected for historical confirmed legacy records created before spool material classification. Such records may legitimately contain a valid diameter and weight transaction but omit `wire_type`.

For legacy records only:

- confirmed record: non-zero diameter remains required;
- `wire_type` may be absent and is represented as UNKNOWN by history/costing;
- if `wire_type` is present, the common parser still restricts it to `CU` or `AL`;
- legacy PENDING/ABORTED still must not contain a conductor material snapshot;
- new KG_FIRST records still require explicit `CU`/`AL` and a non-zero diameter.

No legacy exact-spool provenance rule was weakened.

## Commits

- `7e07c980` — make repair costing consume the authoritative dual-schema write-off codec.
- `164ad8fb` — add regression guards preventing return of the legacy spool-only costing parser.
- `100926d9` — preserve historical confirmed legacy records without `wire_type` as UNKNOWN.

## Safety invariants

- No automatic wire deduction was introduced.
- `RUN_COMPLETED` remains evidence only.
- Exact run duplicate protection is unchanged.
- KG_FIRST SPOOL remains tied to immutable selected spool identity.
- KG_FIRST UNALLOCATED never mutates spool stock.

## Verification state

Code and static contracts were updated, but a green build/CI result is not asserted until GitHub Actions/status evidence is visible for the current head.

## Next priority

Continue mixed-history consumer review and then the existing NDJSON growth/performance/rotation strategy review without premature database migration.
