# Активная работа и следующие шаги

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 138

```text
97-116 CRM / Material Request / Motor / Client / Cash
117-127 exact-spool bridge + atomic RUN_WIRE + accounting/provenance convergence
128-130 obsolete direct writeoff API/types/implementations removed
131-134 warehouse fail-closed reads + single-pass movement summary
135 MaterialLedger public repair/state/currency lookups require explicit found
136 dead adjustmentExists full-log helper removed
137 private repair wrapper + dead usageExists/restoreQuantity helpers removed
138 RepairCosting repair lookup wrapper removed; load() explicit found
```

Verified checkpoint-138 evidence:

```text
3c62d73d3cbd24fe08013cee63a59a8353af3e50
959f8d283e1669f083d9361b11af9a194154fee4
ESP32 Build #1595  32973529504 / SUCCESS
CMP Tests #3622    32973582094 / SUCCESS
```

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

MaterialLedger and RepairCosting now avoid ambiguous one-argument repair lookup wrappers. `savePricing()` still delegates repair identity validation to `load()` so the pricing write path does not add a duplicate `repairs.ndjson` scan.

## Current active queue — bounded runtime/API optimization

1. Audit small read-only/runtime stores for convenience wrappers that collapse read/integrity failure with missing data.
2. Audit dead private helpers that scan full NDJSON files without callers.
3. Prefer narrow source files; do not rewrite critical job/provenance paths merely for API cosmetics.
4. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
5. Keep diagnostics read-only; automatic cleanup/rotation/deletion remains disabled; no premature DB migration.
6. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
