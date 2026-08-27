# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **143**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse summary and the first finalization write-off coverage batch reuse authoritative movement validation. Winding-session persistence retains a pre-begin directory scan only where `JobSpoolSelectionStore::begin()` may mutate recoverable temp evidence.

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
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 140 removes duplicate snapshot/state directory preflight passes. Checkpoint 141 prevents CRM read/integrity failure from becoming false not-found. Checkpoint 142 hides an unused snapshot convenience helper. Checkpoint 143 keeps only the checked autonomous assignment result path public; bool-only assignment and completed-task lookup remain internal.

Latest verified evidence:

```text
6f69d0c548303cb9f6920b602cc0d8754deb5d5b  final source through 143
38e892edc6a45d9540188516d06c6e41c93abd5d  final contract
ESP32 Build #1616   33034665123 / SUCCESS
CMP Tests #3663     33034665166 / SUCCESS
CMP Tests #3664     33034707952 / SUCCESS
```

Earlier `#3657` startup failure and stuck `#3658/#1613` runs were GitHub Actions infrastructure failures before normal job execution; later successful runs above contain and validate checkpoints 141-143.

Checkpoint: `143_AUTONOMOUS_ASSIGNMENT_API_NARROWING_2026-08-27.md`.

## Current NEXT

1. `WindingJournal` is the highest-value remaining growing-file hot spot: `save()` and `loadSessionState()` can perform several full scans of `events.ndjson` for session evidence.
2. Replace those repeated reads with one authoritative streamed/bounded session-analysis pass while preserving duplicate/replay, active-run, highest-run, completed-run and session-context integrity semantics.
3. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
4. Preserve single-pass costing ownership and Web HTTP preflight semantics.
5. No automatic production-data rotation/deletion/truncation and no premature DB migration.
6. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
