# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **135**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse summary is single-pass through authoritative movement validation, and MaterialLedger public repair/material lookups now expose explicit `found` outputs.

## Latest GREEN state

```text
130 direct legacy Store mutation implementations removed
131 warehouse repair lookup -> explicit found
132 warehouse spool identity + wire catalogue -> explicit found/count
133 warehouse price public read -> explicit configured
134 warehouse summary -> authoritative movement audit + aggregation in one primary codec pass
135 MaterialLedger public repair/state/currency reads -> explicit found; dead material wrappers removed
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 135 keeps `repairExists(repairId)` only as a private internal compatibility wrapper for `confirmUsage()`. Public callers must use `repairExists(repairId, found)`. One-argument material state/currency wrappers are fully removed.

Latest verified checkpoint-135 evidence:

```text
6d1ca9a32611b0d0fc42ce4ed2aa1aa22e5d98d9  MaterialLedger public surface narrowed
4f92afd94aec4eca0c2fda4ec6ad7d13c9065e9c  dead material wrappers removed
1c9e2c6b402b28130ffd9e67d12a29b6918476e2  fail-closed lookup contracts
ESP32 Build #1587   32971695182 / SUCCESS
CMP Tests #3601     32971743951 / SUCCESS
```

Checkpoint: `135_MATERIAL_LEDGER_FAIL_CLOSED_LOOKUPS_2026-08-26.md`.

## Current NEXT

1. Continue bounded runtime/API audit for concrete duplicate validated passes or ambiguous read APIs.
2. Keep `MaterialLedgerWeb::handleUsage()` material preflight while it supplies distinct HTTP 404/409 semantics; `confirmUsage()` must still re-read authoritative state immediately before mutation.
3. No automatic production-data rotation/deletion/truncation and no premature DB migration.
4. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
