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

Workshop Web/CRM redesign is GREEN through Cash Web. Coordinated wire-accounting migration is GREEN through checkpoint 121: the physical-spool ↔ MaterialLedger identity bridge now has an explicit crash-safe operator RUN_WIRE ISSUE path.

## Latest GREEN migration state

Checkpoints 118–121 establish:

```text
spool_id <-> warehouse_item_id + CU/AL + exact diameter
/data/warehouse/spool-material-bridges.ndjson

explicit operator RUN_WIRE ISSUE
material_request_id
source_session_id + source_run_id
exact spool_id
actual consumed grams
one durable high-level recovery owner
```

MaterialLedger wire metadata remains backward-compatible:

```text
wire_type = CU | AL
diameter_hundredths_mm = exact diameter
wire metadata requires unit = GRAM
```

Checkpoint 121 production path:

```text
POST /api/material-requests/warehouse
confirmed=true
movement_kind=ISSUE
source_kind=RUN_WIRE
unit=KG
spool_id=<exact spool>
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
- RUN_WIRE returns before the generic Material Request warehouse coordinator, so Ledger-only fallback is impossible.

Latest verified evidence:

```text
source commit        db643d33cd5327556429e71f3734864c484d2f40
final test commit    7e73e9016c690e3ec65dfacfe3a80328b05a2148
ESP32 Build #1551    32951550134 / SUCCESS
CMP Tests #3475      32951582879 / SUCCESS
```

Checkpoint: `121_RUN_WIRE_ISSUE_TRANSACTION_2026-08-26.md`.

## Current NEXT

Audit downstream operator UI and Material Request / wire reporting for the atomic RUN_WIRE ISSUE path. The next block must make the explicit exact-spool transaction usable and visible end-to-end while preserving the existing strict finalization evidence.

Target operator/report provenance remains:

```text
material_request_id
source_session_id + source_run_id
exact physical spool_id
warehouse_item_id through bridge
CU/AL + exact diameter
actual consumed weight
manual confirmation
transaction_ref
```

The legacy direct exact-spool writeoff path remains available until the UI/report migration is coherently GREEN. Do not turn `RUN_COMPLETED` into an automatic stock event.

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

Not complete. Full two-board E2E remains mandatory after CRM/material/writeoff contracts stabilize.
