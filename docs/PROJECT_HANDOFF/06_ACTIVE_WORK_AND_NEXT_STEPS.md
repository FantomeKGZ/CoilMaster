# Активная работа и следующие шаги

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 140

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
```

Verified checkpoint-140 evidence:

```text
1584672e49288334da531235e3bec9f6a691fc7f
0e3786c2894cd5b078645ae24ed5ceb3975cb4ea
ESP32 Build #1606  32981707495 / SUCCESS
CMP Tests #3644    32981785788 / SUCCESS
```

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Finalization write-off coverage remains fixed at 32 targets per page. Winding-session persistence no longer pre-scans snapshot/state directories before `begin()` because those stores do not recover temp files; their normal content passes validate them once. Spool-selection retains read-only preflight because its `begin()` may promote validated temp evidence.

## Current active queue — bounded growing-file optimization

1. Audit remaining append-only readers for concrete duplicate full scans of the same authoritative file.
2. Distinguish mutation-sensitive preflight from read-only validation; do not retain duplicate pre-scans where `begin()` cannot change persisted data.
3. Prefer returning bounded evidence/aggregates from an existing authoritative parser rather than adding parallel parsers.
4. Preserve fixed-size RAM batches; do not replace them with unbounded vectors or whole-file buffering.
5. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
6. Keep diagnostics read-only; automatic cleanup/rotation/deletion remains disabled; no premature DB migration.
7. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
