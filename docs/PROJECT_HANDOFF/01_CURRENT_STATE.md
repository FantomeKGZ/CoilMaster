# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **133**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse lookup APIs now preserve explicit fail-closed result channels for repair existence, spool existence, wire catalogue count and price configuration.

## Latest GREEN state

```text
130 direct legacy Store mutation implementations removed
131 repairExists(id) wrapper removed -> repairExists(id, found)
132 spool identity/count-only catalogue wrappers removed -> explicit found/count
133 loadWarehousePrice(price) removed from public API -> explicit configured form public
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Public price read boundary:

```text
bool loadWarehousePrice(price, configured)
false                   = storage/integrity failure
true + configured=false = valid store, price not configured
```

The old one-argument wrapper remains private implementation-only for now; no production caller can use it.

Latest verified checkpoint-133 evidence:

```text
fe0992219e50800e0dbf181b365d5788f76fc445  price wrapper private
70f987ba58becb1d43ff744bc4ceb02c9cedc0bf  explicit configured-form contract
ESP32 Build #1580   32966647349 / SUCCESS
CMP Tests #3584     32966706823 / SUCCESS
```

Checkpoint: `133_WAREHOUSE_PRICE_LOOKUP_VISIBILITY_2026-08-26.md`.

## Current NEXT

1. Audit warehouse summary for duplicate full `movements.ndjson` passes.
2. Merge validation + summary aggregation only if fail-closed movement integrity remains identical.
3. No automatic production-data rotation/deletion/truncation and no premature DB migration.
4. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
