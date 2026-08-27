# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 151

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
148 managed RUN_WIRE spool mutation removes redundant pre-scan; checked rewrite owns exact before/after identity in one EOF pass
149 SpoolMaterialBridgeStore append combines duplicate-spool detection + next bridge id in one validated bridge-log pass
150 MaterialLedger usage two-pass materials scan intentionally retained: WAL preflight snapshot and mutation-time authoritative rewrite are distinct safety phases; no shared writer lock exists
151 RepairRegistry / WarehouseStore+WriteOff / autonomous assignment append audit found no same-ledger duplicate full scan; production source unchanged
```

Checkpoint 152 production change is committed and under CI verification:

```text
1e831cde072f6c6152d10b7e71cb6a1e0f2a7b0e  perf(archive): reuse latest event on completion
```

Verified latest evidence:

```text
CMP Tests #3709    33041047723 / SUCCESS
CMP Tests #3710    33041065259 / SUCCESS
CMP Tests #3711    33041330947 / SUCCESS
CMP Tests #3712    33041352037 / SUCCESS  (checkpoint 152 target handoff HEAD 2a33d25...)
```

Checkpoint 152 verification at last direct check:

```text
CMP Tests #3713    33041657749 / in_progress
ESP32 Build #1628  33041657768 / in_progress
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

Checkpoint 148 removes the extra full `spools.ndjson` read immediately before managed RUN_WIRE spool rewrite. The rewrite pass itself keeps strict spool ordering/schema, exact spool id, ACTIVE state, exact diameter/material, exact immutable before-state or the one allowed idempotent after-state, EOF validation, atomic replacement/recovery and post-confirm verification.

Checkpoint 149 removes the `loadBySpool() + nextBridgeId()` double full read from `SpoolMaterialBridgeStore::append()`. One private analyzer now provides both duplicate-spool detection and global next id while validating the complete `spool-material-bridges.ndjson`. The read-only `loadBySpool()` path intentionally does not require a next id, so exhausted `bridge_id` space blocks new append fail-closed without corrupting lookup availability.

Checkpoint 150 audits `MaterialLedger::confirmUsage()` and deliberately makes no production source change. The first `materials.ndjson` pass establishes the stock/price/currency snapshot written into `usage.pending`; after the WAL is durable, `rewriteQuantity()` re-reads the authoritative current material file and derives the actual before/after values immediately before atomic replacement. `adjustMaterial()` is an independent writer with its own pending transaction, and there is no common mutex/semaphore covering all material mutations. Fusing these phases into one pre-WAL prepared temp would weaken TOCTOU protection and is therefore rejected.

Checkpoint 151 continues the bounded append audit without forcing an optimization. `RepairRegistry` append operations use one validated pass of their target ledger; client/motor/repair/status lookups that coexist in the same operation are separate registry domains. `WarehouseStore::addSpool()` has one `spools.ndjson` next-id/integrity pass, while write-off has one movement-ledger integrity/id pass plus a separate authoritative spool rewrite. `AutonomousWindingArchive::assignMotorChecked()` has one completed-events proof plus one assignment-ledger next-id/integrity pass. These are not duplicate scans of the same growing file, so production source remains unchanged.

Checkpoint 152 removes the duplicate bounded-tail read in `AutonomousWindingArchive::save(RUN_COMPLETED)`. The save path now calls `loadLastEvent()` once and derives replay/conflict classification and `start_observed` from that same latest record. Behavior is preserved for empty archive, newer completion without observed START, normal START->COMPLETE, duplicate event, conflicting same-run payload and stale run/session ordering. The 512-byte tail bound and fail-closed malformed/unterminated handling remain unchanged. Existing private helper methods are intentionally left in place for this checkpoint to keep the production delta narrow; cleanup is optional only after CI proves the behavioral refactor.

The generic Material Request warehouse coordinator remains unchanged: its repeated scans are across separate integrity ledgers or distinct pre/post mutation phases and are safety-relevant.

## Current active queue — checkpoint 152 verification / next bounded audit

1. Confirm CMP #3713 and ESP32 Build #1628 for commit `1e831cde...`; do not call checkpoint 152 GREEN before direct confirmation.
2. If GREEN, mark checkpoint 152 GREEN in handoff and continue from current branch source.
3. Optional cleanup of now-unused private replay/start helper methods is allowed only if it reduces code without changing semantics; otherwise leave them for a later bounded cleanup pass.
4. Skip MaterialLedger `confirmUsage()` for 2->1 optimization unless a future architecture adds one common material-writer lock spanning preflight through atomic swap and proves equivalent recovery semantics.
5. Keep separate-ledger validation when different files prove different integrity domains.
6. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
7. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
8. Keep diagnostics read-only; no automatic cleanup/rotation/deletion/truncation and no premature DB/index migration.
9. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
