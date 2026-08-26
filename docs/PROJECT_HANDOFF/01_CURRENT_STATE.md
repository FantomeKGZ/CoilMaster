# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth / stable baseline

Working source-of-truth only `cmp-protocol-v1`. `main` не использовать как source.

```text
stable pre-CRM: 449570d47649d5f6336a31ee3eed491256e0fb1a
main -> same commit
stable-2026-08-25-pre-crm-redesign -> same commit
```

## Current phase

Workshop Web/CRM redesign is GREEN through Cash Web. Coordinated wire-accounting migration is GREEN through checkpoint 122: the crash-safe atomic RUN_WIRE transaction now has the authoritative desktop/mobile operator path.

## Latest GREEN migration state

Checkpoints 118–122 establish:

```text
spool_id <-> warehouse_item_id + CU/AL + exact diameter
/data/warehouse/spool-material-bridges.ndjson

explicit operator RUN_WIRE ISSUE
material_request_id
source_session_id + source_run_id
exact spool_id
actual consumed grams
one durable high-level recovery owner
one shared desktop/mobile atomic operator controller
```

MaterialLedger wire metadata remains backward-compatible:

```text
wire_type = CU | AL
diameter_hundredths_mm = exact diameter
wire metadata requires unit = GRAM
```

Production operator path:

```text
RUN_COMPLETED (evidence only)
-> operator selects exact DRAFT/ISSUED Material Request
-> immutable session spool + active spool + exact bridge
-> explicit POST /api/material-requests/warehouse
   confirmed=true
   movement_kind=ISSUE
   source_kind=RUN_WIRE
   unit=KG
   material_request_id=<explicit selection>
   warehouse_item_id=<bridged item>
   source_session_id + source_run_id
   spool_id=<exact immutable spool>
   CU|AL + exact diameter
   quantity_milli_units=<actual consumed grams>
```

Properties:
- explicit operator confirmation remains mandatory;
- exact Material Request ↔ repair ownership is validated;
- exact immutable `JobSpoolSelection` is required;
- exact `source_session_id + source_run_id` must be Completed;
- exact spool-material bridge is required;
- ACTIVE physical spool and ACTIVE MaterialLedger wire item must agree on CU/AL + diameter;
- `quantity_milli_units` is actual consumed grams for RUN_WIRE KG flow;
- one `/data/workshop/run-wire-issue.pending.json` owns cross-store recovery;
- Material Request movement is immutable and carries the same transaction ref;
- MaterialLedger usage is tagged `RWI_TX=<transaction_ref>`;
- standard warehouse KG_FIRST PENDING/CONFIRMED evidence remains the physical writeoff evidence;
- exact physical spool before/after state is part of recovery;
- impossible ordering or unknown spool weight fails closed;
- backup/restore is Busy while RUN_WIRE pending/tmp exists;
- shared desktop/mobile UI no longer POSTs directly to `/api/warehouse/write-offs`;
- legacy writeoff API remains compatibility/history/fault-recovery surface;
- RUN_WIRE returns before the generic Material Request warehouse coordinator, so Ledger-only fallback is impossible.

Latest verified checkpoint-122 evidence:

```text
mobile source commit 81aa293470626436c09fd444eb684bb0b42a0fdb
shared source commit 5c28fadd4a3d1ef8de272f677e2b2f53bfc77794
final test commit   f8d25c1b5fb04bddbd0c2b93fca704f14a7b565f
ESP32 Build #1555   32954324723 / SUCCESS
ESP32 Build #1556   32954467677 / SUCCESS
CMP Tests #3489     32954794059 / SUCCESS
Reference #101/#102 32954324914 / 32954467670 / SUCCESS
```

Checkpoint: `122_ATOMIC_RUN_WIRE_OPERATOR_UI_2026-08-26.md`.

## Current NEXT

Audit downstream read/report provenance and double-accounting boundaries for the atomic RUN_WIRE operator path.

Target report/audit provenance:

```text
material_request_id
transaction_ref
source_session_id + source_run_id
exact physical spool_id
warehouse_item_id through bridge
CU/AL + exact diameter
actual consumed weight
manual confirmation
one physical warehouse movement
one MaterialLedger usage
one Material Request movement
```

Next software block must verify reporting/costing/finalization consume the authoritative physical writeoff evidence exactly once and do not double-count the MaterialLedger usage. Add explicit duplicate protection/contract coverage between compatibility direct writeoff and atomic RUN_WIRE before considering retirement of legacy operator semantics.

## Safety invariants

Never weaken:

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts material;
- warehouse ISSUE requires explicit operator action;
- cash operations never trigger machine or warehouse mutation;
- run-linked wire movement preserves exact run + spool provenance;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- backup/restore cannot cross unfinished RUN_WIRE recovery;
- no automatic production-data deletion/truncation.

## Hardware acceptance

Not complete. Full two-board E2E remains mandatory after CRM/material/writeoff/report contracts stabilize.
