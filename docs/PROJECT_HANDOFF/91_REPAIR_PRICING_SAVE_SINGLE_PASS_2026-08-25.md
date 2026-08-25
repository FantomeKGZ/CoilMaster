# 91 — Repair pricing save single-pass — 2026-08-25

## Scope

Narrow Stage-1 ESP32/storage performance review after block 89.

`RepairCosting::savePricing()` previously performed a full `/data/workshop/repairs.ndjson` identity scan through `repairExists(repairId)` and then called `RepairCosting::load(repairId, current)`, whose first authoritative gate performs the same `repairExists(repairId)` scan again.

This was a proven duplicate authoritative scan with equivalent repair identity semantics in one pricing-save operation.

## Change

Removed the standalone `repairExists(repairId)` call from the initial `savePricing()` argument gate.

Repair identity validation remains fail-closed inside the mandatory `load()` call before any pricing append.

The existing repair lifecycle OPEN gate remains unchanged and still runs before `load()`.

## Preserved semantics

Still required before a pricing revision is appended:

- service/storage readiness;
- non-zero repair ID;
- valid 3-character currency and timestamp;
- repair lifecycle must be OPEN;
- `RepairCosting::load()` must succeed;
- `load()` still requires exact authoritative repair identity validation;
- warehouse movement integrity/provenance aggregation remains authoritative inside `load()`;
- currency consistency remains exact;
- duplicate no-op pricing revision remains rejected;
- pricing revision counter overflow remains rejected.

No persistence schema, pricing arithmetic, material/writeoff provenance, physical START, SSR, UART, Hall or reboot semantics changed.

## I/O effect

For one successful pricing-save attempt, repair identity filesystem scans change from:

```text
2
```

to:

```text
1
```

No cache or unbounded RAM index was introduced.

## Commits

```text
8339863f4c8fe395f5340ec93f98f3f5ac7ef43f  perf(esp32): avoid duplicate repair scan on pricing save
510f449de040ec4aec4814a08fbe7565fcd4c41a  test(esp32): protect pricing save repair single-pass
bbca869a52db892305a1419230c77f26d6def7fd  ci(esp32): audit pricing save repair single-pass
```

## Regression

New guard:

```text
Tests/Web/check_repair_pricing_save_single_pass.js
```

CMP workflow step:

```text
Audit repair pricing save single-pass contracts
```

The guard requires:

- no direct `repairExists()` call inside `savePricing()`;
- lifecycle OPEN gate retained;
- mandatory `load()` retained;
- `load()` still owns exact repair identity validation;
- authoritative warehouse movement integrity aggregation retained.

## Verification — GREEN

Implementation / CI gates:

```text
ESP32 Build #1442
run 32830427142
head 8339863f4c8fe395f5340ec93f98f3f5ac7ef43f
SUCCESS

CMP Protocol Tests #3101
run 32830427181
head 8339863f4c8fe395f5340ec93f98f3f5ac7ef43f
SUCCESS

CMP Protocol Tests #3102
run 32830457634
head 510f449de040ec4aec4814a08fbe7565fcd4c41a
SUCCESS

CMP Protocol Tests #3103
run 32830498664
head bbca869a52db892305a1419230c77f26d6def7fd
SUCCESS
```

Subsequent documentation descendants were also explicitly checked GREEN:

```text
CMP #3104 / run 32830566684 / head 3d4eb23eba67dee5c5b58d1ac6fd8207674ba2fe / SUCCESS
CMP #3105 / run 32830602296 / head ed7e3b7d665435c9aa84fb0d48fcd63e76b94a31 / SUCCESS
CMP #3106 / run 32830643211 / head 58b22be71dad3d9c82f2e41938fe66bbec6292bc / SUCCESS
CMP #3107 / run 32830676575 / head 7c2d7589150296cdca1b7818c94b403252421db2 / SUCCESS
CMP #3108 / run 32830798548 / head e6cfc57d535220b887f18b01fef3e41ad9b92803 / SUCCESS
CMP #3109 / run 32830836275 / head eb20ae239550d2cf163bf5645e85901504fbe05c / SUCCESS
```

Block 91 is fully software GREEN. The latest explicitly verified CMP descendant before this documentation update is `eb20ae239550d2cf163bf5645e85901504fbe05c`.

Hardware testing is not required for this repo-only optimization block.
