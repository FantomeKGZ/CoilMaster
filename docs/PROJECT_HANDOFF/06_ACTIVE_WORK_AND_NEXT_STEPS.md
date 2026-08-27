# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 144

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
```

Verified latest evidence:

```text
baa17db71b3d259d94d22009847c402ae8e6d24c
b186c085ca58743c77b26da33cbd5d795f126126
ESP32 Build #1618  33035231152 / SUCCESS
CMP Tests #3673    33035231146 / SUCCESS
CMP Tests #3674    33035275493 / SUCCESS
```

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Checkpoint 144 replaces runtime WindingJournal multi-pass scans with one fixed-memory streamed analyzer. Exact replay ordering, immutable schema-2 context checks, active-run pairing, monotonic START ids and completed-run evidence remain fail-closed. Boot structure/context validation is intentionally still separate.

## Current active queue — bounded growing-file optimization

1. Continue auditing append-only runtime readers for concrete repeated full-file scans after WindingJournal runtime consolidation.
2. Review the two boot-only WindingJournal scans (`validateJournalStructure` + `validateJournalSessionContexts`) separately. Merge only if one pass can preserve full schema validation, schema-2 session ordering and exact context consistency without introducing parallel parser ownership.
3. Audit autonomous archive runtime save/assignment paths for repeated `events.ndjson` reads now that public API narrowing is complete.
4. Prefer authoritative parser/audit reuse over parallel custom parsers.
5. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
6. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
7. Keep diagnostics read-only; no automatic cleanup/rotation/deletion/truncation and no premature DB migration.
8. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
