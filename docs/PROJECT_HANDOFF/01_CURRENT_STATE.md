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

Workshop Web/CRM redesign is GREEN through Cash Web. Coordinated wire-accounting migration is GREEN through checkpoint 129: atomic RUN_WIRE has authoritative operator UI, single-count costing, bounded cross-log integrity, one agreed KG price, direct exact-spool read provenance, hard-disabled legacy public writeoff POST, persisted spool integrity and narrowed legacy Store API/types.

## Latest GREEN migration state

Checkpoints 118–129 establish:

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
legacy direct Store mutation methods = private
legacy direct request/result support types = private
```

Production operator path remains:

```text
RUN_COMPLETED (evidence only)
-> operator selects exact DRAFT/ISSUED Material Request
-> immutable session spool + ACTIVE spool + exact bridge
-> explicit POST /api/material-requests/warehouse
-> immutable Material Request movement
-> MaterialLedger usage tagged RWI_TX=<transaction_ref>
-> standard physical warehouse CONFIRMED writeoff
```

The bounded `/api/material-requests/movements` read path returns immutable movement JSON directly. New RUN_WIRE rows already expose exact `material_request_id + transaction_ref + warehouse_item_id + source_session_id + source_run_id + spool_id + material_class + diameter`; no additional report join/full-log scan is required.

Public compatibility boundary remains:

```text
POST /api/warehouse/write-offs -> 410 legacy_writeoff_post_disabled
write_performed = false
replacement = /api/material-requests/warehouse
GET /api/warehouse/write-offs -> preserved history/coverage
```

Latest verified checkpoint-129 evidence:

```text
b2f7f13f88bf2a5e489999fdf318523fc1fcdf46  legacy support type narrowing
071e55923ead09264a801c250e8807a17823eba1  mandatory private-type contract
ESP32 Build #1572   32963035385 / SUCCESS
CMP Tests #3551     32963113298 / SUCCESS
```

Checkpoint: `129_WAREHOUSE_LEGACY_SUPPORT_TYPES_NARROWING_2026-08-26.md`.

## Current NEXT

1. Continue compile-proven removal/narrowing of dead legacy warehouse helpers only where deterministic recovery does not depend on them.
2. Keep direct immutable movement fields as the preferred bounded RUN_WIRE report source.
3. Do not add redundant full-log scans or duplicate cross-log joins.
4. Continue software/integrity optimization before final hardware E2E.

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
