# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN production foundation through checkpoint 154

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
152 AutonomousWindingArchive RUN_COMPLETED replay/start classification reuses one loadLastEvent() bounded-tail read
153 dead autonomous helper audit: compiler function/data sections + linker gc-sections already strip unused helpers; NO-CHANGE
154 autonomous archive page query parsed once; candidate filtering compares already-parsed numeric turns directly
```

Production commits:

```text
1e831cde072f6c6152d10b7e71cb6a1e0f2a7b0e  checkpoint 152: reuse latest event on completion
a2f98cb377873d88d3fd103b6dfdfbabaf28ea65  checkpoint 154: parse task query once per page
```

Verified latest evidence:

```text
CMP Tests #3711    33041330947 / SUCCESS
CMP Tests #3712    33041352037 / SUCCESS
CMP Tests #3713    33041657749 / SUCCESS
ESP32 Build #1628  33041657768 / SUCCESS
CMP Tests #3714    33041705508 / SUCCESS
CMP Tests #3715    33041725972 / SUCCESS
CMP Tests #3716    33041762521 / SUCCESS
CMP Tests #3717    33041783094 / SUCCESS
CMP Tests #3720    33042574144 / SUCCESS  (checkpoint 154 production commit a2f98cb...)
ESP32 Build #1629  33042574134 / SUCCESS  (checkpoint 154 production commit a2f98cb...)
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

Checkpoint 152 removes the duplicate bounded-tail read in `AutonomousWindingArchive::save(RUN_COMPLETED)`. The save path now calls `loadLastEvent()` once and derives replay/conflict classification and `start_observed` from that same latest record. Behavior is preserved for empty archive, newer completion without observed START, normal START->COMPLETE, duplicate event, conflicting same-run payload and stale run/session ordering. The 512-byte tail bound and fail-closed malformed/unterminated handling remain unchanged.

Checkpoint 153 checks the remaining now-unused private `findEventReplay()` / `matchingStartExists()` helpers instead of deleting them blindly. ESP32 Build #1628 records `espressif32@7.0.1`, Arduino framework `3.20017.241212+sha.dcc1105b`, and xtensa toolchain `8.4.0+2021r2-patch5`. The exact `platformio-build-esp32.py` at framework commit `dcc1105b` applies `-ffunction-sections`, `-fdata-sections`, and linker `-Wl,--gc-sections`. With no live call sites, the helpers do not contribute to the linked firmware image. Deleting them would only reduce source text, so checkpoint 153 is deliberately NO-CHANGE.

Checkpoint 154 optimizes the live autonomous archive page/filter path. The old filter built `programText(task.event)` for every candidate and then `programMatches()` parsed both that generated String and the unchanged `programQuery` again. The new path parses `programQuery` once before the events scan into a fixed `uint16_t[MaxCoils]` buffer and compares each already-parsed event directly using the same percentage tolerance formula. It preserves the query grammar/limits, full event validation, START/COMPLETE pairing, cursor boundary checks, bounded page storage, assignment resolution and JSON output. CMP #3720 and ESP32 #1629 directly verify the production commit.

The generic Material Request warehouse coordinator remains unchanged: its repeated scans are across separate integrity ledgers or distinct pre/post mutation phases and are safety-relevant.

## Current active queue — checkpoint 155

1. Continue from current `cmp-protocol-v1` with the next measurable runtime/storage/flash candidate.
2. Continue only with measurable runtime/storage/flash candidates; do not spend optimization checkpoints on source-only cleanup already removed by linker GC.
3. Prefer repeated same-operation parsing/read elimination in live paths or fixed-memory aggregate/tail techniques where historical integrity is not weakened.
4. Do not convert authoritative historical integrity scans to tail-only reads merely for speed.
5. Skip MaterialLedger `confirmUsage()` for 2->1 optimization unless a future architecture adds one common material-writer lock spanning preflight through atomic swap and proves equivalent recovery semantics.
6. Keep separate-ledger validation when different files prove different integrity domains.
7. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
8. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery; no automatic cleanup/rotation/deletion/truncation or premature DB/index migration.
9. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
