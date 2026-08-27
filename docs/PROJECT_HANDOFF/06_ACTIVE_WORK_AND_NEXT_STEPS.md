# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 148

```text
97-116 CRM / Material Request / Motor / Client / Cash
117-127 exact-spool bridge + atomic RUN_WIRE + accounting/provenance convergence
128-138 legacy writeoff cleanup + warehouse/material/costing fail-closed reads
139 finalization write-off coverage shares authoritative movement audit
140 winding-session preflight limited to mutation-sensitive spool selection
141 CRM client/motor existence lookups use explicit found
142 unused JobSnapshotStore exists helper removed
143 autonomous assignment public API narrowed
144 WindingJournal runtime evidence shares one streamed pass
145 WindingJournal boot schema/context validation shares one pass
146 CashPaymentStore correction existence + next id share one pass
147 MaterialRequestStatusStore current state + next transition id share one pass
148 managed RUN_WIRE spool mutation removes redundant pre-scan; checked rewrite owns exact before/after identity in one EOF pass
```

Verified latest evidence:

```text
d7242c05357f11266a202008ca7230f6329f1fc6
CMP Tests #3701    33038706783 / SUCCESS
ESP32 Build #1625  33038706784 / SUCCESS
```

CMP host audit remains 69 mandatory steps.

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Checkpoint 148 removes the extra full `spools.ndjson` read immediately before managed RUN_WIRE spool rewrite. The rewrite pass itself now validates strict spool ordering/schema, exact spool id, ACTIVE state, exact diameter/material, exact immutable before-state or the one allowed idempotent after-state, and continues to EOF before accepting replay or entering the existing temp/backup atomic replacement. Separate preflight, post-mutation CONFIRMED verification, swap recovery and exact source-run provenance remain intact.

The generic Material Request warehouse coordinator was audited at the same boundary: its remaining scans are across distinct integrity ledgers or distinct pre/post mutation phases and must stay separate.

## Current active queue — bounded growing-file optimization

1. Checkpoint 149 candidate confirmed: `SpoolMaterialBridgeStore::append()` currently performs `loadBySpool()` and then `nextBridgeId()` over the same `spool-material-bridges.ndjson`; combine duplicate-spool detection plus global next bridge id into one full validated pass.
2. Audit other frequent append-only stores after the bridge store; do not force changes where only one scan exists.
3. Keep separate-ledger validation when separate files prove different integrity domains.
4. Prefer explicit result channels over bool-only convenience existence APIs.
5. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
6. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
7. Keep diagnostics read-only; no automatic cleanup/rotation/deletion/truncation and no premature DB/index migration.
8. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
