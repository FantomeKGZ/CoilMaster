# 89 — Repair pricing reference batching — 2026-08-25

## Scope

Stage-1 ESP32/storage performance cleanup after the GREEN material reference batching block.

`RepairPricingIntegrityAudit::check()` previously reopened and fully scanned `/data/workshop/repairs.ndjson` for every non-empty pricing revision row in `/data/repairs/pricing.ndjson` through `repairExists()`.

That preserved exact reference integrity, but its reference I/O scaled as `N` full repair scans for `N` pricing rows.

## Change

Pricing repair references are now resolved in a fixed-size batch of 32:

- `ReferenceBatchSize = 32`;
- each pricing row still receives full flat-JSON, monetary, currency, and timestamp validation on the authoritative outer pricing pass;
- each batch stores only `repair_id` plus a bounded match counter;
- one complete `repairs.ndjson` identity scan resolves up to 32 pricing references;
- the final partial batch is always resolved before success.

Worst-case repair-reference filesystem scans therefore change from:

```text
N
```

to:

```text
ceil(N / 32)
```

No unbounded RAM index or optimistic persistence cache was introduced.

## Preserved fail-closed semantics

For every pricing reference, the resolver still requires exactly one matching persisted repair row:

```text
matches == 1
```

Therefore all of the following still fail closed:

- missing referenced repair;
- zero/invalid repair ID;
- malformed repair identity row;
- duplicate persisted repair ID;
- invalid pricing JSON/schema fields.

Multiple pricing revisions referencing the same valid repair remain allowed; each reference independently resolves to the same single authoritative repair row.

Workshop full-schema validation remains owned by the authoritative workshop/business audit. The repeated reference pass intentionally stays identity-focused to avoid duplicate parser work.

## Deliberate KEEP findings

During this review:

- `repair-status.ndjson` bounded self-scan remains KEEP because CLOSED entries may appear in operator order rather than sorted `repair_id` order; removing exact uniqueness checks would require unbounded RAM or a stronger persisted ordering invariant that does not currently exist.
- autonomous winding assignment reference validation remains KEEP at batch size 32 because assignments may point to old completed runs in arbitrary operator order.

## Safety

No physical START, SSR, UART, Hall, run-completion, spool provenance, repair closure, or manual wire write-off semantics changed.

## Regression / CI

- `Tests/Web/check_repair_pricing_reference_batching.js`
- CMP workflow step: `Audit repair pricing reference batching contracts`

Required software verification:

- ESP32 Build SUCCESS on implementation commit `29ecbb799a14da455aa5d732764613465b21788a` or descendant;
- CMP Protocol Tests SUCCESS on CI commit `91ea3ee824b4589e88ec2c2cd7c063ad3e3c7ffe` or descendant;
- explicit `Audit repair pricing reference batching contracts` SUCCESS.

Hardware testing is not required for this repo-only optimization block.
