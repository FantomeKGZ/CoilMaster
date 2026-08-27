# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

Production behavior is GREEN through checkpoint **154**. Checkpoint 153 is a completed NO-CHANGE linker/flash audit. Checkpoint **155** has a production runtime optimization committed and is under direct CMP + ESP32 verification: motor-similarity matching now parses the candidate winding program once and each persisted motor program once instead of reparsing both through `valid() + equivalent()` for every motor. Atomic RUN_WIRE remains the only current wire mutation path. Checkpoint 150 remains a deliberate NO-CHANGE safety boundary for MaterialLedger usage.

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
154 autonomous task page filter: parse programQuery once per page and compare numeric turns directly; removes per-candidate program String construction + candidate/query reparsing
155 motor similarity: candidate winding program parsed once, each persisted program parsed once; removes per-motor duplicate validation/equivalence reparsing
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

Checkpoint 154 optimizes `AutonomousWindingArchive::appendTasksPageJson()`. Previously each candidate task created a canonical `programText()` String, reparsed that generated candidate String, and reparsed the same `programQuery` inside `programMatches()`. The page path now parses the query once into a fixed `uint16_t` turns array and compares each already-parsed `RemoteWindingEvent::turns[]` directly with the same numeric tolerance formula. Query grammar, maximum 10 coils / 9999 turns, event record validation, START/COMPLETE pairing, cursor boundaries, assignment resolution and output JSON are unchanged. Production commit: `a2f98cb377873d88d3fd103b6dfdfbabaf28ea65`.

Checkpoint 155 optimizes `RepairRegistry::appendSimilarMotorsJson()`. Previously the candidate program was validated once, then `WindingProgramParser::equivalent()` reparsed that unchanged candidate for every persisted motor; each persisted `coil_program` was also parsed once by `valid()` and again by `equivalent()`. The path now parses the candidate once into fixed `uint16_t[10]`, parses each persisted program once, and compares count/turn values directly. Parser grammar and bounds remain identical, malformed persisted programs still fail closed, identity scoring/output JSON are unchanged, and no whole-file buffering is introduced. Production commit: `415394d162de0f1c83e433cbbea3db94833b3162`.

Latest verified evidence:

```text
CMP Tests #3711     33041330947 / SUCCESS
CMP Tests #3712     33041352037 / SUCCESS
CMP Tests #3713     33041657749 / SUCCESS
ESP32 Build #1628   33041657768 / SUCCESS
CMP Tests #3714     33041705508 / SUCCESS
CMP Tests #3715     33041725972 / SUCCESS
CMP Tests #3716     33041762521 / SUCCESS
CMP Tests #3717     33041783094 / SUCCESS
CMP Tests #3718     33042461751 / SUCCESS
CMP Tests #3719     33042491867 / SUCCESS
CMP Tests #3720     33042574144 / SUCCESS  (checkpoint 154 production commit a2f98cb...)
ESP32 Build #1629   33042574134 / SUCCESS  (checkpoint 154 production commit a2f98cb...)
CMP Tests #3721     33042622904 / SUCCESS
CMP Tests #3722     33042647618 / SUCCESS
CMP Tests #3723     33042699795 / SUCCESS
CMP Tests #3724     33042727200 / SUCCESS
```

Checkpoint 155 verification started on `415394d...`:

```text
CMP Tests #3725     33043013899 / queued at last direct check
ESP32 Build #1630   33043013882 / queued at last direct check
```

Earlier supporting GREEN evidence remains in GitHub Actions history. CMP host audit remains 69 mandatory steps.

## Current NEXT

1. Verify CMP #3725 and ESP32 Build #1630 for production commit `415394d...`; do not call checkpoint 155 GREEN before both are directly confirmed.
2. If GREEN, mark checkpoint 155 GREEN and continue checkpoint 156 from current branch source.
3. Continue only with measurable runtime/storage/flash candidates; avoid source-only cleanup already handled by linker GC.
4. Prefer same-operation duplicate parsing/read elimination or fixed-memory aggregate/tail techniques where historical integrity semantics are preserved.
5. Keep separate-ledger scans when they prove different integrity domains or distinct pre/post mutation phases.
6. Keep MaterialLedger usage two-pass unless a future design introduces one shared writer lock spanning preflight through atomic swap and proves equivalent crash recovery.
7. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
8. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation and exact-spool provenance.
9. No automatic production-data rotation/deletion/truncation and no premature DB/index migration.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
