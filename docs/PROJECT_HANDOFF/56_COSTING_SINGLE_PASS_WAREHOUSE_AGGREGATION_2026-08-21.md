# Checkpoint 56 — single-pass warehouse aggregation for costing

Date: 2026-08-21
Branch: `cmp-protocol-v1`

## Closed block

`RepairCosting::load()` no longer performs a second full read of `/data/warehouse/movements.ndjson` after the authoritative warehouse movement integrity audit.

`WarehouseMovementIntegrityAudit` now exposes a read-only repair-scoped aggregation result:

```text
WarehouseMovementRepairTotals
WarehouseMovementIntegrityAudit::checkRepair(...)
```

During the existing primary transaction/schema pass the audit aggregates only final `CONFIRMED` records for the requested `repair_id`:

```text
wire cost
CU/AL/UNKNOWN cost split
CU/AL/UNKNOWN grams
line counts
currency consistency
```

The existing bounded provenance uniqueness pass remains unchanged and authoritative.

## Safety / integrity semantics preserved

- no automatic wire deduction;
- `RUN_COMPLETED` remains eligibility evidence only;
- legacy and KG_FIRST records still use the shared `WarehouseWriteOffRecordCodec` inside the audit;
- only a valid `PENDING -> CONFIRMED` final record contributes to costing;
- `ABORTED` records do not contribute;
- mixed currencies for the same repair fail closed;
- overflow checks and nearest-minor-unit wire pricing remain enforced;
- provenance uniqueness still rejects legacy session conflicts and duplicate exact runs.

## Performance effect

Before:

```text
RepairCosting::load
→ full WarehouseMovementIntegrityAudit primary pass
→ bounded provenance pass(es)
→ full movements.ndjson scan again for repair wire totals
```

After:

```text
RepairCosting::load
→ authoritative primary audit pass + repair wire aggregation
→ bounded provenance pass(es)
```

One complete `movements.ndjson` read is removed from every costing request. This also benefits reports that consume `/api/repairs/costing`.

## Commits

```text
3dfa053f  Add repair totals to movement integrity audit
6332b727  Aggregate repair wire totals during movement audit
33d46d32  Reuse movement audit totals in repair costing
0796d164  Guard single-pass wire costing aggregation
```

## Verification status

Static source/contracts were updated. ESP32 build and GitHub CI are not to be reported as successful unless separately observed.

## Next repo-reviewable work

Continue examining growing-file consumers for redundant full scans, especially backup integrity and material/pricing paths. Do not introduce automatic deletion, arbitrary NDJSON rotation thresholds, or database migration without measured runtime evidence.
