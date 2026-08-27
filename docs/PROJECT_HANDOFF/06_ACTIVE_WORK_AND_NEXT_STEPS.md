# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 147

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
```

Verified latest evidence:

```text
a0ec58c552bed883d289fa1e30160f365955a6e1
6c404070d80532e617174d4621d828cf16a094c0
ESP32 Build #1624  33036483178 / SUCCESS
CMP Tests #3695    source commit / SUCCESS
CMP Tests #3696    33036507740 / SUCCESS
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

Checkpoint 147 removes the second full `material-request-status.ndjson` pass from transition persistence. `requestExists()` remains a separate immutable request-catalog read, while `analyzeStatus()` now owns current lifecycle state plus global next transition id.

## Current active queue — bounded growing-file optimization

1. Audit Material Request movement and warehouse coordinator runtime paths for repeated reads of the same append-only file in one operation.
2. Audit other frequent append-only stores after Material Request; do not force changes where only one scan exists.
3. Keep separate-ledger validation when separate files prove different integrity domains.
4. Prefer explicit result channels over bool-only convenience existence APIs.
5. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
6. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
7. Keep diagnostics read-only; no automatic cleanup/rotation/deletion/truncation and no premature DB/index migration.
8. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
