# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 143

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
```

Verified latest evidence:

```text
6f69d0c548303cb9f6920b602cc0d8754deb5d5b
38e892edc6a45d9540188516d06c6e41c93abd5d
ESP32 Build #1616  33034665123 / SUCCESS
CMP Tests #3663    33034665166 / SUCCESS
CMP Tests #3664    33034707952 / SUCCESS
```

Earlier `#3657` startup failure and stuck `#3658/#1613` runs were GitHub Actions infrastructure failures before job execution. Later successful runs above contain and validate checkpoints 141-143.

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Finalization write-off coverage remains fixed at 32 targets per page. Winding-session persistence no longer pre-scans snapshot/state directories before `begin()` because those stores do not recover temp files; their normal content passes validate them once. Spool-selection retains read-only preflight because its `begin()` may promote validated temp evidence.

## Current active queue — WindingJournal single-pass optimization

1. Highest-value next performance block: `WindingJournal::save()` currently scans `/data/winding-runs/events.ndjson` separately for duplicate detection, active-run state, highest run id, matching RUN_STARTED and completed-run count depending on event type.
2. `loadSessionState()` separately performs active/highest/completed scans of the same growing file.
3. Design one authoritative streamed session-analysis pass that can return all required bounded evidence for one target session/run.
4. Preserve exact duplicate/replay semantics, START->COMPLETE transition proof, monotonic run ids, completed-run sequence and immutable session context checks.
5. Do not use unbounded vectors or whole-file buffering; RAM must remain fixed/bounded.
6. Keep Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery unchanged.
7. Keep diagnostics read-only; no automatic cleanup/rotation/deletion/truncation and no premature DB migration.
8. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
