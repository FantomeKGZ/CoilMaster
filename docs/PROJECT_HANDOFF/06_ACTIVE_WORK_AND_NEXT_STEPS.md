# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 146

```text
97-116 CRM / Material Request / Motor / Client / Cash
117-127 exact-spool bridge + atomic RUN_WIRE + accounting/provenance convergence
128-130 obsolete direct writeoff API/types/implementations removed
131-134 warehouse fail-closed reads + single-pass movement summary
135 MaterialLedger public repair/state/currency lookups require explicit found
136 dead adjustmentExists full-log helper removed
137 private repair wrapper + dead usageExists/restoreQuantity helpers removed
138 RepairCosting repair lookup wrapper removed; load() explicit found
139 first bounded finalization coverage batch shares authoritative movement pairing/provenance pass
140 winding-session preflight kept only for mutation-sensitive spool-selection directory
141 CRM client/motor existence lookups use explicit found across registry/Web/cash/intake
142 unused JobSnapshotStore exists helper removed from public API
143 autonomous assignment public API narrowed to assignMotorChecked result semantics
144 WindingJournal runtime save/state evidence shares one streamed journal analysis pass
145 WindingJournal boot schema/context validation shares one combined pass
146 CashPaymentStore correction existence + next event id share one mutation-time pass
```

Verified latest evidence:

```text
e15222e299ed4736a66d577175cf4e381e29747a
a96953ab4a51370dcb6402580def7f2de0256011
ESP32 Build #1622  33035968880 / SUCCESS
CMP Tests #3687    33035968846 / SUCCESS
CMP Tests #3689    33036075195 / SUCCESS
```

CMP host audit now has 69 mandatory steps; all passed in #3689.

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Checkpoint 146 removes the extra full `repair-payments.ndjson` scan that used to precede next-id allocation for correction events. `eventBelongsToRepair(..., found)` remains a separate Web provenance preflight; `analyzeAppendState()` is the mutation-time TOCTOU check.

## Current active queue — bounded growing-file optimization

1. Continue auditing CRM/material-request/cash append-only runtime stores for repeated reads of the same file in one operation.
2. Prioritize frequent mutation/read paths over occasional operator-only scans.
3. Keep separate-ledger validation when separate files prove different integrity domains.
4. Prefer explicit result channels over bool-only convenience existence APIs.
5. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
6. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
7. Keep diagnostics read-only; no automatic cleanup/rotation/deletion/truncation and no premature DB/index migration.
8. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
