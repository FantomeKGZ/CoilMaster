# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **150**. Atomic RUN_WIRE is the only current wire mutation path. Growing-file runtime optimization now covers WindingJournal, CashPaymentStore correction append preparation, Material Request status transitions, managed RUN_WIRE spool mutation, and spool/material bridge append. Checkpoint 150 is a deliberate NO-CHANGE safety boundary for MaterialLedger usage: its two `materials.ndjson` passes are distinct preflight and mutation-time validation phases and must not be fused without a shared writer lock.

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
150 MaterialLedger confirmUsage two-pass materials scan retained: preflight + mutation-time revalidation are distinct safety phases; no shared writer lock exists
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

Checkpoint 150 audits `MaterialLedger::confirmUsage()` and intentionally keeps its two `materials.ndjson` reads. The first pass derives the pre-WAL stock/price/currency snapshot used by `usage.pending`; the later `rewriteQuantity()` pass re-reads the current authoritative file immediately before atomic replacement and derives the committed remaining stock/price/currency from that current state. `adjustMaterial()` is a separate writer with its own pending transaction, and the firmware has no common mutex/semaphore spanning all material writers. Preparing a one-pass temp before WAL would therefore weaken mutation-time TOCTOU protection. No production source change was made for checkpoint 150.

Latest verified evidence:

```text
14ea791ca5741e9aec75d00b80e2c523a34a7d82  final production source through 149; unchanged by checkpoint 150
CMP Tests #3704     33039077049 / SUCCESS
ESP32 Build #1627   33039077052 / SUCCESS
CMP Tests #3705     33039186178 / SUCCESS  (handoff record commit 8cb1093...)
CMP Tests #3706     33039200646 / SUCCESS  (handoff HEAD ee7015f...)
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
4. Keep MaterialLedger usage two-pass unless a future design introduces one shared writer lock spanning preflight through atomic swap and proves equivalent crash recovery.
5. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
6. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation and exact-spool provenance.
7. No automatic production-data rotation/deletion/truncation and no premature DB/index migration.
8. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
