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

Workshop Web/CRM redesign is GREEN through Cash Web. Coordinated wire-accounting migration is GREEN through checkpoint 131: atomic RUN_WIRE is the only current wire mutation path; accounting/provenance is converged and bounded; obsolete direct warehouse mutation code is deleted; repair existence lookup now exposes storage success separately from `found`.

## Latest GREEN migration state

Checkpoints 118–131 establish:

```text
spool_id <-> warehouse_item_id + CU/AL + exact diameter
explicit operator RUN_WIRE ISSUE
material_request_id
source_session_id + source_run_id
exact spool_id
actual consumed grams
one durable high-level recovery owner
one shared desktop/mobile atomic operator controller
one authoritative wire-cost count
cross-log exact transaction integrity
one agreed KG wire price
reserved RWI_TX system provenance
direct spool_id in new immutable RUN_WIRE movements
legacy public writeoff POST = HTTP 410
persisted spool_id == immutable JobSpoolSelection when field exists
historic spool-less RUN_WIRE movement remains compatible
legacy direct Store mutation entrypoints = removed
legacy SpoolWriteOffResult = removed
historical append/recovery helpers = retained
ambiguous repairExists(id) wrapper = removed
fail-closed repairExists(id, found) = authoritative
```

Production operator path remains:

```text
RUN_COMPLETED (evidence only)
-> operator selects exact DRAFT/ISSUED Material Request
-> immutable session spool + ACTIVE spool + exact bridge
-> explicit POST /api/material-requests/warehouse
-> immutable Material Request movement
-> MaterialLedger usage tagged RWI_TX=<transaction_ref>
-> managed physical warehouse PENDING/CONFIRMED evidence
```

Current exact completion/spool/session/run/duplicate safety is owned by `RunWireIssueCoordinator`. Historical `WarehouseStore::recoverPendingWriteOff()` only reconciles already-durable old PENDING state from exact persisted BEFORE/AFTER spool evidence and retained append helpers.

The bounded `/api/material-requests/movements` read path returns immutable movement JSON directly. New RUN_WIRE rows expose exact `material_request_id + transaction_ref + warehouse_item_id + source_session_id + source_run_id + spool_id + material_class + diameter`; no additional report join/full-log scan is required.

Public compatibility boundary remains:

```text
POST /api/warehouse/write-offs -> 410 legacy_writeoff_post_disabled
write_performed = false
replacement = /api/material-requests/warehouse
GET /api/warehouse/write-offs -> preserved history/coverage
```

Latest verified checkpoint-131 evidence:

```text
6ccdec084001faabf25eaf5b28177d8f7e89a7d5  declaration cleanup
279fc281b42a559f89f911e9f0b2758ccd02e8ff  implementation cleanup
d848ce45eb35c7f2bba817d6de1efd0c4f4a02bd  fail-closed lookup contract
ESP32 Build #1576   32964609675 / SUCCESS
CMP Tests #3570     32964670388 / SUCCESS
CMP Tests #3571     32964882464 / SUCCESS
```

Checkpoint: `131_WAREHOUSE_FAIL_CLOSED_REPAIR_LOOKUP_2026-08-26.md`.

## Current NEXT

1. Audit `loadActiveSpoolIdentity`, `loadWarehousePrice`, and `loadKnownWireDiameters` overloads; remove only dead/ambiguous convenience forms.
2. Prefer APIs where storage/read success is distinct from `found/configured/count`.
3. Preserve managed RUN_WIRE, GET/history, integrity, backup and deterministic historical recovery.
4. Do not add redundant full-log scans or duplicate cross-log joins.
5. Continue software/integrity optimization before final hardware E2E.

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
