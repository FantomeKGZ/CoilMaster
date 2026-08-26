# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **140**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse summary and the first finalization write-off coverage batch reuse authoritative movement validation. Winding-session persistence now retains a pre-begin directory scan only where `JobSpoolSelectionStore::begin()` may mutate recoverable temp evidence.

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
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 140 removes duplicate snapshot/state directory preflight passes. `JobSnapshotStore::begin()` and `JobStateStore::begin()` only ensure existing directories, while their content scans already reject temp/non-canonical entries. `JobSpoolSelectionStore::begin()` can recover a validated temp selection, so its directory keeps the read-only preflight before begin.

Latest verified checkpoint-140 evidence:

```text
1584672e49288334da531235e3bec9f6a691fc7f  final source
0e3786c2894cd5b078645ae24ed5ceb3975cb4ea  final contract
ESP32 Build #1606   32981707495 / SUCCESS
CMP Tests #3644     32981785788 / SUCCESS
```

Checkpoint: `140_WINDING_SESSION_SELECTION_ONLY_PREFLIGHT_2026-08-26.md`.

## Current NEXT

1. Continue bounded audit of growing append-only readers for concrete redundant full scans.
2. Preserve mutation-sensitive preflight only where recovery/begin can modify persisted state.
3. Reuse authoritative parsers/audits where possible; keep fixed-size RAM bounds.
4. Preserve single-pass costing ownership and Web HTTP preflight semantics.
5. No automatic production-data rotation/deletion/truncation and no premature DB migration.
6. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
