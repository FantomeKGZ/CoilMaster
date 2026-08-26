# Checkpoint 123 — RUN_WIRE accounting convergence

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Status

**GREEN.** Atomic RUN_WIRE no longer double-counts the same wire consumption through both MaterialLedger usage and the standard confirmed physical warehouse writeoff.

## Defect found

Checkpoint 121 intentionally creates three durable views for one explicit operator RUN_WIRE transaction:

```text
Material Request movement
MaterialLedger usage tagged RWI_TX=<transaction_ref>
standard physical warehouse KG_FIRST PENDING/CONFIRMED movement
```

`RepairCosting::load()` already treated the confirmed warehouse movement as authoritative wire cost, but also summed every MaterialLedger usage into generic material cost. Therefore a completed RUN_WIRE could contribute the same wire value twice:

```text
wireCostMinor      <- confirmed warehouse movement
materialCostMinor  <- same RUN_WIRE MaterialLedger usage
```

## Fix

Authoritative costing boundary is now:

```text
wire cost
  = confirmed physical warehouse writeoff evidence

generic material cost
  = ordinary MaterialLedger usage excluding system-owned RWI_TX usage

total cost
  = wire + generic materials + labour
```

`RepairCosting` still validates every MaterialLedger usage record and its persisted cost formula. A valid internal RUN_WIRE usage is recognized only by the reserved `RWI_TX=<RWI-...>;` transaction marker and is excluded from `materialCostMinor` because the same wire cost is already represented by the authoritative physical warehouse movement.

Malformed RWI markers fail costing closed rather than silently becoming ordinary material rows.

## In-flight transaction safety

Costing now fails closed while either high-level RUN_WIRE recovery file exists:

```text
/data/workshop/run-wire-issue.pending.json
/data/workshop/run-wire-issue.pending.tmp
```

This prevents reports/finalization from publishing a mixed state where MaterialLedger evidence may exist before physical warehouse confirmation is complete.

## Reserved provenance namespace

The generic operator endpoint `POST /api/materials/usage` can no longer create a comment beginning with `RWI_TX=`. It returns an explicit non-mutating error with `write_performed=false`.

Therefore ordinary manual material usage cannot spoof the internal RUN_WIRE tag and disappear from costing. The internal `RunWireIssueCoordinator` remains the owner of `RWI_TX=` evidence.

## Finalization impact

`RepairFinalizationGuard` already consumes `RepairCosting::load()` rather than computing a second independent cost. The corrected costing therefore propagates directly into finalization without adding another scan or another accounting authority.

## Duplicate physical deduction boundary

Both mutation paths use the same exact source-run duplicate evidence:

```text
confirmedWriteOffForSourceRun(source_session_id, source_run_id)
```

- legacy/direct writeoff checks it before mutation;
- atomic RUN_WIRE checks it before saving its high-level pending.

Therefore an already confirmed exact run cannot be physically deducted again by switching mutation path.

## Read/report provenance

Material Request movement history already preserves and exposes:

```text
material_request_id
warehouse_item_id
transaction_ref
source_session_id
source_run_id
material_class
wire_diameter_hundredths_mm
quantity_milli_units
```

No second authoritative report ledger was introduced in checkpoint 123.

## Commits

```text
29e6315c04a3901fd068df60ddc9b9849920d879  fix(materials): reserve RUN_WIRE usage provenance prefix
52e0c629fe1f112ceff373b2e83decf20ff76b21  fix(costing): deduplicate atomic RUN_WIRE ledger usage
02b6e98e56732563df56cb7611b31d4a08086a99  test(costing): enforce RUN_WIRE deduplication
921a2d70856207124343ff0bae3ccc7c7462608a  test(run-wire): reserve ledger transaction provenance
357a7677f7e91bb2a9812462e0aff8c9d0e15ea4  test(run-wire): make reserved provenance assertion semantic
```

## Verified CI evidence

```text
CMP Protocol Tests #3496  32955502176 / SUCCESS
ESP32 Build #1557         32955502232 / SUCCESS
CMP Protocol Tests #3497  32955588708 / SUCCESS
ESP32 Build #1558         32955588907 / SUCCESS
CMP Protocol Tests #3498  32955633833 / SUCCESS
CMP Protocol Tests #3500  32955968429 / SUCCESS
```

`#3499 / 32955668637` failed only in the newly added nested host contract because one assertion compared an unnecessarily fragile escaped JSON fragment. Production source had already built successfully. The assertion was made semantic (`reserved_usage_comment_prefix` + `write_performed` + checked comment assignment), and final `#3500` is GREEN.

## Next

1. Add explicit cross-path contract coverage proving legacy/direct and atomic RUN_WIRE share exact-run duplicate protection.
2. Strengthen cross-log integrity so a persisted `RWI_TX` MaterialLedger usage is accepted as managed accounting evidence only when matching immutable RUN_WIRE Material Request / warehouse evidence exists.
3. Keep report/read provenance bounded and avoid creating a second transaction authority.
4. Continue software integrity/optimization before mandatory final two-board hardware E2E.
