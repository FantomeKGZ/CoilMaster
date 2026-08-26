# Checkpoint 125 — RUN_WIRE price / provenance convergence

Date: **2026-08-26**  
Branch: **`cmp-protocol-v1`**

## Status

**GREEN**

Checkpoint 125 removes the last split price authority from atomic `RUN_WIRE` and reserves the system `RWI_TX=` provenance namespace from generic operator Web inputs.

## One KG wire price

Atomic RUN_WIRE now requires exact equality before the high-level pending intent is persisted:

```text
MaterialLedger wire item price per gram
x 1000
= Material Request RUN_WIRE unit_cost_minor per KG
= WarehousePrice.price_per_kg_minor
```

Currency must also agree.

If this equality is not true, `RunWireIssueCoordinator::buildPending()` fails before `run-wire-issue.pending.json` can be saved.

## Recovery price guard

`executePhysicalPhases()` re-reads the current warehouse price and compares it to the immutable `pending.unitCostMinor` before the physical warehouse phase.

Therefore a warehouse price change after Material Request / Ledger evidence but before physical commit, including after reboot, does not silently create a different physical wire cost. Recovery fails closed instead.

## Persisted cross-log price audit

`CM_RunWireAccountingIntegrityAudit` now validates historical completed RUN_WIRE evidence as well:

- Material Request movement `unit_cost_minor` is the immutable KG price;
- MaterialLedger `price_per_unit_minor` must equal `unit_cost_minor / 1000` for GRAM wire accounting;
- warehouse CONFIRMED `price_per_kg_minor` must equal the same `unit_cost_minor`;
- existing `line_cost_minor == cost_amount_minor` correlation remains mandatory.

Thus a completed transaction with split historical prices fails the broad workshop persistence integrity audit.

## System provenance namespace

`RWI_TX=` is system-owned.

Already before this checkpoint, generic `POST /api/materials/usage` rejected an operator comment beginning with `RWI_TX=`.

Checkpoint 125 adds the same reservation to compatibility `POST /api/warehouse/write-offs`:

```text
comment starts RWI_TX=
-> 400 reserved_writeoff_comment_prefix
-> write_performed=false
```

The managed atomic coordinator does not use this generic Web endpoint, so it can continue to persist legitimate tagged physical evidence internally.

## Commits

```text
74a92901262a060b203ea1b1a3cc3313537ce51a  coordinator one-price pre-pending + recovery guard
84dabb9920cf60ca8cd8745b16a3e97e9093f50b  persisted cross-log price integrity
4f20cc928723a0c3dd873741260ebed07d8690f5  reserve RWI_TX on compatibility writeoff Web
c799c74f3f8f4c6cbcd538ea662e3c86fe039304  mandatory price/provenance contracts
```

## Verified CI

```text
ESP32 Build #1562  32960004843  SUCCESS
CMP Tests #3514    32960004874  SUCCESS
ESP32 Build #1563  32960173338  SUCCESS
CMP Tests #3515    32960173324  SUCCESS
ESP32 Build #1564  32960269882  SUCCESS
CMP Tests #3516    32960270010  SUCCESS
CMP Tests #3517    32960329745  SUCCESS
```

## Safety invariants unchanged

- `RUN_COMPLETED` is evidence only;
- no automatic material deduction;
- wire ISSUE requires explicit operator action;
- exact Material Request + warehouse item + physical spool + source session/run remain mandatory;
- physical START remains local-only;
- no auto-resume after reboot;
- Arduino remains SSR owner;
- ESP32/Web never controls SSR directly.

## Next

Review the formal deprecation boundary for compatibility mutating `POST /api/warehouse/write-offs`. Preserve historical GET/read and internal recovery evidence. Do not remove the legacy backend path until all current runtime/test callers are proven non-production or migrated.
