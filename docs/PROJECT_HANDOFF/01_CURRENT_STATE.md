# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **149**. Atomic RUN_WIRE is the only current wire mutation path. Growing-file runtime optimization now covers WindingJournal, CashPaymentStore correction append preparation, Material Request status transitions, managed RUN_WIRE spool mutation, and spool/material bridge append.

## Latest GREEN state

```text
139 finalization write-off coverage batch fused with authoritative movement audit
140 winding-session preflight limited to mutation-sensitive spool-selection directory
141 CRM client/motor existence -> explicit found
142 JobSnapshotStore exists helper removed from public API
143 autonomous assignment public API narrowed
144 WindingJournal runtime save/state -> one streamed pass
145 WindingJournal boot schema/context -> one combined pass
146 CashPaymentStore correction lookup + next event id -> one mutation-time pass
147 MaterialRequestStatusStore state + next transition id -> one status-journal pass
148 managed RUN_WIRE spool mutation removes redundant pre-scan before checked rewrite
149 SpoolMaterialBridgeStore append duplicate-spool check + next bridge id -> one validated bridge-log pass
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 148 removes the extra full spool-file pre-scan before managed rewrite while keeping exact before/after identity, EOF validation, atomic replacement/recovery and post-confirm verification.

Checkpoint 149 replaces `loadBySpool() + nextBridgeId()` duplicate full reads of `spool-material-bridges.ndjson` during append with one private validated analyzer pass. Public read-only `loadBySpool()` remains fail-closed and does not become unreadable when the global `bridge_id` space is exhausted; only append fails closed on id exhaustion.

Latest verified evidence:

```text
14ea791ca5741e9aec75d00b80e2c523a34a7d82  final source through 149
CMP Tests #3704     33039077049 / SUCCESS
ESP32 Build #1627   33039077052 / SUCCESS
```

Previous supporting GREEN evidence:

```text
CMP Tests #3703     33038913115 / SUCCESS
ESP32 Build #1626   33038913050 / SUCCESS
CMP Tests #3702     33038798586 / SUCCESS
CMP Tests #3701     33038706783 / SUCCESS
ESP32 Build #1625   33038706784 / SUCCESS
```

CMP host audit remains 69 mandatory steps.

## Current NEXT

1. Continue bounded audit of frequent append-only stores for same-operation duplicate full-file scans.
2. Do not change stores that already perform only one validated pass.
3. Keep separate-ledger scans when they prove different integrity domains or distinct pre/post mutation phases.
4. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
5. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation and exact-spool provenance.
6. No automatic production-data rotation/deletion/truncation and no premature DB/index migration.
7. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
