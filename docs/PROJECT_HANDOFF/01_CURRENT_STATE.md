# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

Production behavior is GREEN through checkpoint **152**. Checkpoint **153** is a completed NO-CHANGE linker/flash audit: the now-unused autonomous private replay/start helpers are already dead-stripped by the exact ESP32 toolchain used by CI, so deleting them would be cosmetic source cleanup rather than a measurable firmware optimization. Atomic RUN_WIRE remains the only current wire mutation path. Checkpoint 150 remains a deliberate NO-CHANGE safety boundary for MaterialLedger usage.

## Latest state

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
152 AutonomousWindingArchive save: replay/conflict + RUN_COMPLETED start_observed share one loadLastEvent() bounded-tail read
153 autonomous dead-helper audit: exact ESP32 build uses function/data sections + linker gc-sections, so unused private helpers add no final firmware bytes; NO-CHANGE
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

Checkpoint 152 changes only `AutonomousWindingArchive::save()`. The old RUN_COMPLETED path called `findEventReplay()` and then `matchingStartExists()`, and both reopened `events.ndjson` and parsed the same latest record through `loadLastEvent()`. The save path now loads that latest bounded tail once and derives stale-order rejection, same-run identity/turn conflict, duplicate classification and the `start_observed` flag from the same immutable in-memory latest event. Empty archive and a newer completion without an observed START preserve `start_observed=0`; matching START->COMPLETE produces `start_observed=1`; duplicate/conflict result semantics remain unchanged. The existing 512-byte tail bound and fail-closed malformed/unterminated handling remain unchanged. Production commit: `1e831cde072f6c6152d10b7e71cb6a1e0f2a7b0e`.

Checkpoint 153 verifies whether deleting the now-unused private `findEventReplay()` / `matchingStartExists()` implementations would reduce final firmware. ESP32 Build #1628 records PlatformIO `espressif32@7.0.1`, Arduino framework `3.20017.241212+sha.dcc1105b`, and xtensa toolchain `8.4.0+2021r2-patch5`. The exact framework build script at `dcc1105b` compiles with `-ffunction-sections` / `-fdata-sections` and links with `-Wl,--gc-sections`. Because both helpers have no live call sites after checkpoint 152, their sections are already excluded from the linked image. No production source change is justified for flash/runtime optimization; cosmetic cleanup is intentionally deferred.

Latest verified evidence:

```text
CMP Tests #3711     33041330947 / SUCCESS
CMP Tests #3712     33041352037 / SUCCESS
CMP Tests #3713     33041657749 / SUCCESS  (checkpoint 152 production commit 1e831cd...)
ESP32 Build #1628   33041657768 / SUCCESS  (checkpoint 152 production commit 1e831cd...)
CMP Tests #3714     33041705508 / SUCCESS
CMP Tests #3715     33041725972 / SUCCESS
CMP Tests #3716     33041762521 / SUCCESS
CMP Tests #3717     33041783094 / SUCCESS  (checkpoint 152 handoff HEAD dbd6dc5...)
```

Earlier supporting GREEN evidence remains in GitHub Actions history. CMP host audit remains 69 mandatory steps.

## Current NEXT

1. Checkpoint 154: continue only with a measurable runtime/storage/flash candidate from current branch; do not spend a checkpoint deleting dead code that linker GC already removes.
2. Prefer same-operation duplicate parsing/read elimination or fixed-memory aggregate/tail techniques where historical integrity semantics are preserved.
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
