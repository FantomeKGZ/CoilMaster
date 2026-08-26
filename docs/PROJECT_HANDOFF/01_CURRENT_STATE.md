# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **134**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse lookup APIs are fail-closed, obsolete direct writeoff code is removed, and warehouse summary no longer performs a duplicate manual full parse of `movements.ndjson`.

## Latest GREEN state

```text
130 direct legacy Store mutation implementations removed
131 repair lookup -> explicit found
132 spool identity + wire catalogue -> explicit found/count
133 warehouse price public read -> explicit configured
134 warehouse summary -> authoritative movement audit + aggregation in one primary codec pass
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 134 preserves global exact-run provenance uniqueness. Only the redundant summary parser/pass was removed; bounded provenance verification remains fail-closed.

The old one-argument `loadWarehousePrice(WarehousePrice&)` declaration and implementation are now fully removed. Public price reads use only `loadWarehousePrice(price, configured)`.

Latest verified checkpoint-134 evidence:

```text
e734ad77ee232a9ed17b1118179e1ffd96666cc5  final source
ESP32 Build #1585   32967638219 / SUCCESS
CMP Tests #3594     32967638259 / SUCCESS
```

Checkpoint: `134_WAREHOUSE_MOVEMENT_SUMMARY_SINGLE_PASS_2026-08-26.md`.

## Current NEXT

1. Audit MaterialLedger/runtime for duplicate full-log validation + read passes.
2. Reuse existing validated preflight outputs only where semantics remain fail-closed.
3. No automatic production-data rotation/deletion/truncation and no premature DB migration.
4. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
