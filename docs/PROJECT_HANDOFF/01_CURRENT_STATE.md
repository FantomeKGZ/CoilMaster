# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **145**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse summary and first finalization write-off coverage batch reuse authoritative movement validation. WindingJournal boot and runtime reads are now consolidated to one streamed pass per operation.

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
144 WindingJournal runtime save/state reads -> one streamed session-analysis pass
145 WindingJournal boot schema/context validation -> one combined pass
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 144 removes runtime duplicate full scans of `/data/winding-runs/events.ndjson`. Checkpoint 145 folds boot schema-2 session ordering/context consistency into the existing structure pass. `CM_WindingJournal.cpp` now contains exactly two full journal read sites: combined boot validation and runtime analysis.

Latest verified evidence:

```text
bbbc53d96e535204e38db7da0fc79f872dd5a19a  final source through 145
1760a447ffd216976b844a51adb055a5701f16ff  final contract
ESP32 Build #1620   33035508401 / SUCCESS
CMP Tests #3681     33035532132 / SUCCESS
```

Intermediate CMP #3680 was stale textual-contract failure before the checkpoint-145 contract update.

Checkpoint: `145_WINDING_JOURNAL_BOOT_SINGLE_PASS_2026-08-27.md`.

## Current NEXT

1. Continue bounded audit of other growing append-only stores for concrete same-file duplicate full scans.
2. Prioritize frequent runtime paths; occasional operator-only scans across different ledgers remain acceptable unless metrics justify indexing.
3. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
4. Preserve single-pass costing ownership, Web HTTP preflight semantics and mutation-time TOCTOU validation.
5. No automatic production-data rotation/deletion/truncation and no premature DB migration.
6. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
