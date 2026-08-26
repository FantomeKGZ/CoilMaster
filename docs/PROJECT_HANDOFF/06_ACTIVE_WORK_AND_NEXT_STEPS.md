# Активная работа и следующие шаги

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 139

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
```

Verified checkpoint-139 evidence:

```text
0e4896e0139ac8b7f79effb02d644e42dd057d22
1d78995c5cd203cbfadbaf67aa03b48a813a5ca0
ESP32 Build #1602  32979299677 / SUCCESS
CMP Tests #3635    32979340004 / SUCCESS
```

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Finalization write-off coverage remains fixed at 32 targets per page. The first batch now receives exact coverage evidence during the authoritative movement pairing/global provenance audit, eliminating the old standalone full movement pre-scan. Later pages retain bounded lightweight scans, so RAM use does not grow with history size.

## Current active queue — bounded growing-file optimization

1. Audit remaining append-only readers for concrete duplicate full scans of the same authoritative file.
2. Prefer returning bounded evidence/aggregates from an existing authoritative parser rather than adding parallel parsers.
3. Preserve fixed-size RAM batches; do not replace them with unbounded vectors or whole-file buffering.
4. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
5. Keep diagnostics read-only; automatic cleanup/rotation/deletion remains disabled; no premature DB migration.
6. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
