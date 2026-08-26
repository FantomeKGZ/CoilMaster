# Активная работа и следующие шаги

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 137

```text
97-116 CRM / Material Request / Motor / Client / Cash
117-127 exact-spool bridge + atomic RUN_WIRE + accounting/provenance convergence
128-130 obsolete direct writeoff API/types/implementations removed
131-134 warehouse fail-closed reads + single-pass movement summary
135 MaterialLedger public repair/state/currency lookups require explicit found
136 dead adjustmentExists full-log helper removed
137 private repair wrapper + dead usageExists/restoreQuantity helpers removed
```

Verified checkpoint-137 evidence:

```text
90c732a9caef1d1e4104c9c7374a72f6a8df3811
5dc6f1c0303834274d989b8846b53ba34c1f3368
ESP32 Build #1592  32972822029 / SUCCESS
CMP Tests #3614    32972911974 / SUCCESS
```

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

MaterialLedger now has no ambiguous one-argument repair/state/currency lookup wrappers. Dead private full-log/catalog rewrite helpers are removed; live recovery reads and atomic mutation passes remain intact.

## Current active queue — bounded runtime/API optimization

1. Audit small runtime/read helpers for dead full-log scans, duplicate parsing or bool APIs that collapse read failure with not-found.
2. Prefer narrow source files; avoid large rewrites unless several proven cleanup items justify one exact replacement.
3. Preserve Web HTTP preflight semantics and mutation-time TOCTOU validation.
4. Do not weaken repair-reference, currency, usage, provenance or recovery checks.
5. Keep diagnostics read-only; automatic cleanup/rotation/deletion remains disabled; no premature DB migration.
6. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
