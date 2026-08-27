# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 149

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
149 SpoolMaterialBridgeStore append combines duplicate-spool detection + next bridge id in one validated bridge-log pass
```

Verified latest evidence:

```text
14ea791ca5741e9aec75d00b80e2c523a34a7d82
CMP Tests #3704    33039077049 / SUCCESS
ESP32 Build #1627  33039077052 / SUCCESS
```

Supporting GREEN evidence:

```text
CMP Tests #3703    33038913115 / SUCCESS
ESP32 Build #1626  33038913050 / SUCCESS
CMP Tests #3702    33038798586 / SUCCESS
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

Checkpoint 148 removes the extra full `spools.ndjson` read immediately before managed RUN_WIRE spool rewrite. The rewrite pass itself keeps strict spool ordering/schema, exact spool id, ACTIVE state, exact diameter/material, exact immutable before-state or the one allowed idempotent after-state, EOF validation, atomic replacement/recovery and post-confirm verification.

Checkpoint 149 removes the `loadBySpool() + nextBridgeId()` double full read from `SpoolMaterialBridgeStore::append()`. One private analyzer now provides both duplicate-spool detection and global next id while validating the complete `spool-material-bridges.ndjson`. The read-only `loadBySpool()` path intentionally does not require a next id, so exhausted `bridge_id` space blocks new append fail-closed without corrupting lookup availability.

The generic Material Request warehouse coordinator remains unchanged: its repeated scans are across separate integrity ledgers or distinct pre/post mutation phases and are safety-relevant.

## Current active queue — bounded growing-file optimization

1. Audit the next frequent append-only stores for two full reads of the same NDJSON during one mutation; change only confirmed duplicates.
2. MaterialRequestStore append has already been checked at this boundary and performs only one request-log scan; do not change it for this optimization.
3. Keep separate-ledger validation when different files prove different integrity domains.
4. Prefer explicit result channels over bool-only convenience existence APIs.
5. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
6. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
7. Keep diagnostics read-only; no automatic cleanup/rotation/deletion/truncation and no premature DB/index migration.
8. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
