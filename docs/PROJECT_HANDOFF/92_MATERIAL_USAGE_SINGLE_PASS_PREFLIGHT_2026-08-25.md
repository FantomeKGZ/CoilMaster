# 92 — Material usage single-pass preflight — 2026-08-25

## Scope

Narrow Stage-1 ESP32/storage performance review after block 91.

`MaterialLedger::confirmUsage()` previously performed two full authoritative reads of `/data/materials/materials.ndjson` before the actual mutation pass:

1. `readStockQuantity()` scanned the catalog for current stock;
2. `confirmUsage()` reopened the same catalog and scanned it again for price, currency and ACTIVE status.

The existing `MaterialLedger::readMaterialState()` already provides stock, price, currency and ACTIVE validation in one fail-closed full pass.

## Change

`confirmUsage()` now calls:

```text
readMaterialState(usage.materialId, stockBefore, price, currency)
```

instead of the separate stock-only scan plus direct pricing/status scan.

The KGS currency gate and insufficient-stock rejection remain explicit in `confirmUsage()`.

The later `rewriteQuantity()` pass remains unchanged because it is the actual transactional mutation/revalidation boundary.

## Preserved semantics

`readMaterialState()` remains fail-closed and validates:

- non-zero target material ID;
- flat JSON validity for every material row;
- strictly increasing material IDs;
- stock field presence;
- non-zero price;
- exactly 3-character currency;
- status field presence;
- exact single target match;
- target must be `ACTIVE`.

`confirmUsage()` still requires:

- ready storage and valid repair/material/quantity/timestamp;
- exact repair existence;
- repair lifecycle OPEN;
- no pending usage transaction;
- monotonically allocated usage ID;
- sufficient stock;
- KGS currency;
- durable pending-usage journal before mutation;
- `rewriteQuantity()` transaction pass;
- durable usage append;
- pending journal removal only after success.

No schema, costing arithmetic, writeoff provenance, physical START, SSR, UART, Hall, reboot or restore semantics changed.

## I/O effect

Material catalog passes for one successful usage confirmation change from:

```text
preflight stock scan
+ preflight pricing/status scan
+ mutation/rewrite scan
= 3 passes
```

to:

```text
combined material-state preflight scan
+ mutation/rewrite scan
= 2 passes
```

No cache or unbounded RAM index was introduced.

## Commits

```text
72401aae0d1b34fbb211ce92c48d0a367f337b91  perf(esp32): collapse material usage preflight scan
8ce55052f98d491f3f1f2fda4830955e87159798  test(esp32): protect material usage single-pass preflight
6d77ac1b4ad7fcc25cc1873d5e0c13e819011ece  ci(esp32): audit material usage single-pass preflight
```

## Regression

New guard:

```text
Tests/Web/check_material_usage_single_pass.js
```

CMP workflow step:

```text
Audit material usage single-pass preflight contracts
```

The guard requires:

- `confirmUsage()` uses `readMaterialState()`;
- no separate `readStockQuantity()` preflight inside `confirmUsage()`;
- no direct reopen of `MaterialsPath` for pricing preflight inside `confirmUsage()`;
- KGS gate retained;
- `rewriteQuantity()` mutation pass retained;
- `readMaterialState()` retains its fail-closed identity/stock/price/currency/status contracts.

## Verification — GREEN

```text
ESP32 Build #1443
run 32831517073
head 72401aae0d1b34fbb211ce92c48d0a367f337b91
SUCCESS

CMP Protocol Tests #3111
run 32831517018
head 72401aae0d1b34fbb211ce92c48d0a367f337b91
SUCCESS

CMP Protocol Tests #3112
run 32831547926
head 8ce55052f98d491f3f1f2fda4830955e87159798
SUCCESS

CMP Protocol Tests #3113
run 32831593193
head 6d77ac1b4ad7fcc25cc1873d5e0c13e819011ece
SUCCESS
```

Block 92 is fully software GREEN.

No intermediate hardware test is required for this repo-only optimization block.
