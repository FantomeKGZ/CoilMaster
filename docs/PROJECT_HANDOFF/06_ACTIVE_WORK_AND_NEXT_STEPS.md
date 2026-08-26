# Активная работа и следующие шаги

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 134

```text
97-116 CRM / Material Request / Motor / Client / Cash
117-127 exact-spool bridge + atomic RUN_WIRE + accounting/provenance convergence
128-130 obsolete direct writeoff API/types/implementations narrowed then removed
131 repair existence lookup made explicitly fail-closed
132 spool identity and wire catalogue lookups made explicitly fail-closed
133 public price lookup requires explicit configured output
134 warehouse movement summary aggregation shares authoritative movement codec/integrity pass
```

Verified checkpoint-134 evidence:

```text
e734ad77ee232a9ed17b1118179e1ffd96666cc5
ESP32 Build #1585  32967638219 / SUCCESS
CMP Tests #3594    32967638259 / SUCCESS
```

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Warehouse summary no longer performs a duplicate manual full `movements.ndjson` parser pass. `checkSummary()` validates transaction pairing through `WarehouseWriteOffRecordCodec`, accumulates month/all-time totals, and still runs bounded global confirmed provenance uniqueness.

## Current active queue — MaterialLedger/runtime scan audit

1. Inspect `CM_MaterialLedger.cpp`, adjustment/currency/reference helpers and their mandatory contracts for duplicate validated full-log scans.
2. Respect the existing checkpoint-92 material-usage single-pass preflight; do not duplicate or undo it.
3. Optimize only a concrete repeated pass where the same authoritative parser can safely return the required evidence/aggregate.
4. Do not weaken repair-reference, currency, usage or atomic-recovery integrity checks.
5. Keep diagnostics read-only and automatic cleanup/rotation/deletion disabled; no premature DB migration.
6. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
