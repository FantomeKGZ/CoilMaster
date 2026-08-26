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

Workshop Web/CRM redesign is GREEN through Cash Web. The coordinated wire-accounting migration is now in progress.

## Latest GREEN migration state

Checkpoints 118–120 now establish the physical-spool ↔ generic-material identity boundary:

```text
spool_id <-> warehouse_item_id + CU/AL + exact diameter
/data/warehouse/spool-material-bridges.ndjson
```

MaterialLedger wire metadata is backward-compatible:

```text
wire_type = CU | AL
diameter_hundredths_mm = exact diameter
wire metadata requires unit = GRAM
```

Checkpoint 120 adds explicit operator-only runtime creation:

```text
POST /api/warehouse/spool-material-bridges
```

Properties:
- requires `spool_id + warehouse_item_id + confirm=1 + linked_at`;
- physical spool must exist and be ACTIVE;
- MaterialLedger item must exist and be ACTIVE;
- MaterialLedger item must be `GRAM` with structured wire metadata;
- exact CU/AL + diameter must match between both authoritative records;
- already-bridged spool is rejected fail-closed;
- server derives metadata from authoritative records, not caller fields;
- successful creation appends bridge identity evidence only;
- response states `stock_mutated:false`;
- no current writeoff/usage/machine mutation is invoked.

Latest evidence on tree `fa651e3e50a25df9489db24b6c71bd853171a9b8`:

```text
CMP Protocol Tests 32944119683 / SUCCESS
ESP32 Build         32944119688 / SUCCESS
```

## Current NEXT

Build the crash-safe explicit-operator run-linked wire accounting migration toward Material Request `RUN_WIRE`. Before coding mutation, inspect existing MaterialLedger usage/adjustment transaction semantics and the current Material Request warehouse coordinator so physical spool mutation, MaterialLedger movement and Material Request evidence cannot diverge across a reboot/crash window.

Target provenance remains:

```text
material_request_id
source_session_id + source_run_id
exact physical spool_id provenance through bridge
CU/AL + exact diameter
actual consumed weight
manual confirmation
```

Current exact-spool writeoff/finalization remains authoritative until the coordinated transition across movement/costing/finalization/backup/integrity/reports/Web/tests is complete.

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
- run-linked wire movement preserves exact run provenance;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Hardware acceptance

Not complete. Full two-board E2E remains mandatory after CRM/material/writeoff contracts stabilize.
