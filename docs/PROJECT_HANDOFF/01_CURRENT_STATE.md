# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **144**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse summary and first finalization write-off coverage batch reuse authoritative movement validation. Winding runtime journal state/transition evidence now shares one streamed pass over the growing event log.

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
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 144 removes runtime duplicate full scans of `/data/winding-runs/events.ndjson`: new RUN_STARTED/RUN_COMPLETED persistence and `loadSessionState()` obtain active/highest/completed/replay/start/context evidence in one bounded streamed pass. Boot structure and cross-session-context audits remain separate and fail closed.

Latest verified evidence:

```text
baa17db71b3d259d94d22009847c402ae8e6d24c  final source through 144
b186c085ca58743c77b26da33cbd5d795f126126  final contract
ESP32 Build #1618   33035231152 / SUCCESS
CMP Tests #3673     33035231146 / SUCCESS
CMP Tests #3674     33035275493 / SUCCESS
```

Checkpoint: `144_WINDING_JOURNAL_RUNTIME_SINGLE_PASS_2026-08-27.md`.

## Current NEXT

1. Continue bounded audit of append-only runtime readers for remaining concrete duplicate full scans.
2. Audit boot-only WindingJournal structure/context passes separately; combine only if one authoritative pass can preserve the same startup integrity semantics.
3. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
4. Preserve single-pass costing ownership, Web HTTP preflight semantics and mutation-time TOCTOU validation.
5. No automatic production-data rotation/deletion/truncation and no premature DB migration.
6. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
