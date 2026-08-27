# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **146**. Atomic RUN_WIRE is the only current wire mutation path. WindingJournal boot/runtime reads are consolidated, and CashPaymentStore correction append preparation now uses one mutation-time journal pass.

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
139 first bounded finalization coverage batch fused with authoritative movement pairing/provenance audit
140 winding-session preflight limited to mutation-sensitive spool-selection directory
141 CRM client/motor existence -> explicit found across registry/Web/cash/intake
142 JobSnapshotStore exists helper removed from public API
143 autonomous assignment public API narrowed to assignMotorChecked result semantics
144 WindingJournal runtime save/state reads -> one streamed session-analysis pass
145 WindingJournal boot schema/context validation -> one combined pass
146 CashPaymentStore correction lookup + next event id -> one mutation-time pass
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 146 removes the bool-only public `eventExists()` helper and fuses correction-reference existence with monotonic next-id derivation in `analyzeAppendState()`. The Web layer still validates exact repair/client ownership separately before append, preserving HTTP semantics and mutation-time TOCTOU validation.

Latest verified evidence:

```text
e15222e299ed4736a66d577175cf4e381e29747a  final source through 146
a96953ab4a51370dcb6402580def7f2de0256011  final mandatory contract/workflow
ESP32 Build #1622   33035968880 / SUCCESS
CMP Tests #3687     33035968846 / SUCCESS
CMP Tests #3689     33036075195 / SUCCESS
```

CMP host audit count is now 69 mandatory steps; all passed in #3689.

Checkpoint: `146_CASH_PAYMENT_APPEND_SINGLE_PASS_2026-08-27.md`.

## Current NEXT

1. Continue bounded audit of CRM/material-request/cash append-only stores for remaining same-file duplicate scans.
2. Keep separate-ledger scans where they protect different integrity domains.
3. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
4. Preserve Web HTTP preflight semantics and mutation-time TOCTOU validation.
5. No automatic production-data rotation/deletion/truncation and no premature DB/index migration.
6. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
