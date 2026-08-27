# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **147**. Atomic RUN_WIRE is the only current wire mutation path. Growing-file runtime optimization now covers WindingJournal, CashPaymentStore correction append preparation, and Material Request status transitions.

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
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 147 replaces `resolve() + nextTransitionId()` duplicate status-journal reads with private `analyzeStatus()`. Public `resolve(..., found)` remains fail-closed and uses the same analyzer. Immutable request existence still reads the separate request catalog because it proves a separate integrity boundary.

Latest verified evidence:

```text
a0ec58c552bed883d289fa1e30160f365955a6e1  final source through 147
6c404070d80532e617174d4621d828cf16a094c0  final contract
ESP32 Build #1624   33036483178 / SUCCESS
CMP Tests #3695     source commit / SUCCESS
CMP Tests #3696     33036507740 / SUCCESS
```

CMP host audit remains 69 mandatory steps.

Checkpoint: `147_MATERIAL_REQUEST_STATUS_TRANSITION_SINGLE_PASS_2026-08-27.md`.

## Current NEXT

1. Audit Material Request movement/coordinator runtime paths for same-file duplicate scans.
2. Keep separate-ledger scans when they prove different integrity domains.
3. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
4. Preserve Web HTTP preflight semantics and mutation-time TOCTOU validation.
5. No automatic production-data rotation/deletion/truncation and no premature DB/index migration.
6. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
