# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **138**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse summary is single-pass, MaterialLedger public lookups are explicitly fail-closed, dead private ledger helpers are removed, and RepairCosting repair identity validation no longer uses an ambiguous bool wrapper.

## Latest GREEN state

```text
130 direct legacy Store mutation implementations removed
131 warehouse repair lookup -> explicit found
132 warehouse spool identity + wire catalogue -> explicit found/count
133 warehouse price public read -> explicit configured
134 warehouse summary -> one authoritative movement validation/aggregation pass
135 MaterialLedger public repair/state/currency -> explicit found
136 dead adjustmentExists full-log helper removed
137 one-arg MaterialLedger repair + dead usageExists/restoreQuantity removed
138 RepairCosting one-arg repair wrapper removed; load() uses explicit found
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

`RepairCosting::savePricing()` still reuses `load()` and does not perform a duplicate repair identity scan. `load()` now rejects both repair reference read/integrity failure and `found=false` explicitly before costing scans.

Latest verified checkpoint-138 evidence:

```text
3c62d73d3cbd24fe08013cee63a59a8353af3e50  final RepairCosting source
959f8d283e1669f083d9361b11af9a194154fee4  single-pass/fail-closed contract
ESP32 Build #1595   32973529504 / SUCCESS
CMP Tests #3622     32973582094 / SUCCESS
```

Checkpoint: `138_REPAIR_COSTING_FAIL_CLOSED_REPAIR_LOOKUP_2026-08-26.md`.

## Current NEXT

1. Continue bounded runtime/read-helper audit for dead scans or ambiguous bool APIs.
2. Preserve single-pass costing ownership and Web HTTP preflight semantics.
3. No automatic production-data rotation/deletion/truncation and no premature DB migration.
4. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
