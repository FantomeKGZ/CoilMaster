# Checkpoint 51 — NDJSON growth observability before rotation

Date: 2026-08-20
Branch: `cmp-protocol-v1`

## Why this checkpoint exists

The current append-oriented persistence is still appropriate for the device, but several authoritative reads scale with journal size. In particular, warehouse movement integrity performs a full transaction validation plus a provenance uniqueness pass; exact write-off lookups and costing call authoritative validation before consuming the journal.

A premature split/rotation would be risky because exact source-run provenance, repair costing, finalization coverage, backup integrity, and historical reporting all depend on complete durable history.

The next performance step is therefore measurement first, not migration.

## New read-only diagnostics

`GET /api/system/storage` now reports file sizes for the append-oriented files most relevant to scan cost:

- `warehouse_movements_bytes` — `/data/warehouse/movements.ndjson`
- `winding_events_bytes` — `/data/winding-runs/events.ndjson`
- `repair_registry_bytes` — `/data/workshop/repairs.ndjson`
- `wire_spools_bytes` — `/data/warehouse/spools.ndjson`

It also reports `ndjson_growth_monitoring_only: true` and retains `automatic_cleanup_allowed: false`.

No file is modified, truncated, compacted, deleted, or rotated by this diagnostics path.

## Operator visibility

The system diagnostics UI now displays:

- Журнал списаний
- Журнал намоток
- Реестр ремонтов
- Реестр бухт

alongside total/free microSD capacity.

This makes it possible to correlate actual device latency with real production data size before selecting a rotation threshold or index strategy.

## Current known hot path

`WarehouseMovementIntegrityAudit` intentionally remains fail-closed. Its provenance uniqueness validation can become the dominant cost as `movements.ndjson` grows. Exact write-off lookup currently performs the authoritative audit and then a second linear scan for the requested source run.

This checkpoint does **not** cache or skip that audit because doing so without a durable generation/index contract would weaken corruption and duplicate detection.

## Safe future strategy

Before any rotation implementation, hardware/runtime measurements should capture at least:

1. `warehouse_movements_bytes`;
2. `winding_events_bytes`;
3. elapsed response time for warehouse summary/history and costing;
4. free heap / minimum free heap;
5. reboot recovery behavior with the same data set.

If rotation becomes necessary, the design must preserve an authoritative cross-segment identity/provenance view. A simple "rename old NDJSON and start empty" approach is not acceptable because it can permit duplicate exact-run write-off coverage or make old repair finalization/reporting incomplete.

A database migration is not required at this stage.

## Commits

- `922284e5` — expose read-only sizes of critical growing journals/registries.
- `70bc90b0` — show those sizes in system diagnostics UI.
- `6677b369` — static guard for growth telemetry and read-only/no-auto-cleanup semantics.
- `36e5d950` — run the new guard in CMP protocol CI.

## Verification state

The new guard is wired into CI, but a green CI/build result must not be claimed until GitHub status/workflow evidence is visible for the current HEAD.

## Next work

Continue runtime/fault-path acceptance and collect real-device growth/latency measurements before implementing any NDJSON rotation or durable side index.
