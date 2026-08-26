# Активная работа и следующие шаги

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 135

```text
97-116 CRM / Material Request / Motor / Client / Cash
117-127 exact-spool bridge + atomic RUN_WIRE + accounting/provenance convergence
128-130 obsolete direct writeoff API/types/implementations narrowed then removed
131 warehouse repair existence lookup made explicitly fail-closed
132 warehouse spool identity and wire catalogue lookups made explicitly fail-closed
133 public warehouse price lookup requires explicit configured output
134 warehouse movement summary aggregation shares authoritative movement codec/integrity pass
135 MaterialLedger public repair/state/currency lookups require explicit found outputs
```

Verified checkpoint-135 evidence:

```text
6d1ca9a32611b0d0fc42ce4ed2aa1aa22e5d98d9
4f92afd94aec4eca0c2fda4ec6ad7d13c9065e9c
1c9e2c6b402b28130ffd9e67d12a29b6918476e2
ESP32 Build #1587  32971695182 / SUCCESS
CMP Tests #3601    32971743951 / SUCCESS
```

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Public MaterialLedger lookup boundary now preserves separate read-success and existence results. The remaining one-argument repair wrapper is private-only for the current internal `confirmUsage()` call; one-argument material state/currency wrappers are removed.

## Current active queue — bounded runtime/API optimization

1. Continue searching for concrete duplicate validated full-log passes or ambiguous read APIs.
2. Do not remove `MaterialLedgerWeb::handleUsage()` material preflight while it is required to distinguish HTTP `material_not_found` and unsupported-currency responses; `confirmUsage()` still needs its own authoritative mutation preflight against TOCTOU.
3. Consider full removal of private `MaterialLedger::repairExists(repairId)` only through a safe exact rewrite of `CM_MaterialLedger.cpp`; do not rewrite a large source solely for cosmetic cleanup.
4. Do not weaken repair-reference, currency, usage, provenance or atomic-recovery checks.
5. Keep diagnostics read-only and automatic cleanup/rotation/deletion disabled; no premature DB migration.
6. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
