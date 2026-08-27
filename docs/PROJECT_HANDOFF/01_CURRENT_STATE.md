# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **151**. Atomic RUN_WIRE is the only current wire mutation path. Growing-file runtime optimization now covers WindingJournal, CashPaymentStore correction append preparation, Material Request status transitions, managed RUN_WIRE spool mutation, and spool/material bridge append. Checkpoint 150 is a deliberate NO-CHANGE safety boundary for MaterialLedger usage: its two `materials.ndjson` passes are distinct preflight and mutation-time validation phases and must not be fused without a shared writer lock. Checkpoint 151 extends the bounded append-only audit and deliberately makes no production change because the inspected remaining paths do not contain a same-operation duplicate full scan of the same growing NDJSON.

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
151 bounded append-only audit: RepairRegistry, WarehouseStore/WriteOff and autonomous assignment inspected; no safe same-ledger duplicate full-scan candidate found, production source unchanged
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

Checkpoint 151 audits the next append-oriented storage paths against the same rule. `RepairRegistry` uses one validated pass of the target client/motor/repair/status ledger per append need, with additional reads only from different registry domains. `WarehouseStore::addSpool()` uses one validated `spools.ndjson` pass to derive the next id before append; warehouse write-off keeps one movement-ledger id/integrity scan plus a separate authoritative spool rewrite, which are different ledgers/phases. `AutonomousWindingArchive::assignMotorChecked()` keeps one completed-event proof plus one assignment-ledger next-id/integrity scan, again different ledgers. No same-operation duplicate full scan of one growing NDJSON was found, so production source is unchanged.

Latest verified evidence:

```text
14ea791ca5741e9aec75d00b80e2c523a34a7d82  final production source through 149; unchanged by checkpoints 150-151
CMP Tests #3704     33039077049 / SUCCESS
ESP32 Build #1627   33039077052 / SUCCESS
CMP Tests #3705     33039186178 / SUCCESS
CMP Tests #3706     33039200646 / SUCCESS
CMP Tests #3707     33039762821 / SUCCESS  (checkpoint 150 handoff record)
CMP Tests #3708     33039779059 / SUCCESS  (checkpoint 150 handoff HEAD f2103cf...)
CMP Tests #3709     33041047723 / SUCCESS  (checkpoint 151 handoff record a71446a...)
CMP Tests #3710     33041065259 / SUCCESS  (checkpoint 151 handoff HEAD 3604181...)
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

1. Checkpoint 152: remove the duplicate bounded-tail read in `AutonomousWindingArchive::save(RUN_COMPLETED)` only if replay/conflict classification and `start_observed` can be derived from one `loadLastEvent()` without changing fail-closed semantics.
2. Continue bounded audit only where a same-operation duplicate read is demonstrably present; do not force code changes.
3. Do not change stores that already perform only one validated pass of the target ledger.
4. Keep separate-ledger scans when they prove different integrity domains or distinct pre/post mutation phases.
5. Keep MaterialLedger usage two-pass unless a future design introduces one shared writer lock spanning preflight through atomic swap and proves equivalent crash recovery.
6. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
7. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation and exact-spool provenance.
8. No automatic production-data rotation/deletion/truncation and no premature DB/index migration.
9. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
