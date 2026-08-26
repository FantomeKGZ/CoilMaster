# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **137**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse summary is single-pass, MaterialLedger public lookups are explicitly fail-closed, and obsolete private full-log helpers are removed.

## Latest GREEN state

```text
130 direct legacy Store mutation implementations removed
131 warehouse repair lookup -> explicit found
132 warehouse spool identity + wire catalogue -> explicit found/count
133 warehouse price public read -> explicit configured
134 warehouse summary -> one authoritative movement validation/aggregation pass
135 MaterialLedger public repair/state/currency -> explicit found
136 dead adjustmentExists full-log helper removed
137 one-arg repair wrapper + dead usageExists/restoreQuantity helpers removed
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

`confirmUsage()` now explicitly distinguishes repair-reference read failure from `found=false` before mutation. Usage recovery still uses `readStockQuantity()` for exact BEFORE/AFTER reconciliation; this live recovery helper was intentionally retained.

Latest verified checkpoint-137 evidence:

```text
90c732a9caef1d1e4104c9c7374a72f6a8df3811  final MaterialLedger source
5dc6f1c0303834274d989b8846b53ba34c1f3368  final contracts
ESP32 Build #1592   32972822029 / SUCCESS
CMP Tests #3614     32972911974 / SUCCESS
```

Checkpoint: `137_MATERIAL_LEDGER_RETIRED_PRIVATE_HELPERS_REMOVAL_2026-08-26.md`.

## Current NEXT

1. Continue bounded runtime/read-helper audit for dead scans or ambiguous bool APIs.
2. Preserve Web HTTP preflight semantics and mutation-time TOCTOU protection.
3. No automatic production-data rotation/deletion/truncation and no premature DB migration.
4. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
