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

Workshop Web/CRM redesign is GREEN through Cash Web. Coordinated wire-accounting migration is GREEN through checkpoint 126: atomic RUN_WIRE has authoritative operator UI, single-count costing, bounded cross-log integrity, one agreed KG price, direct exact-spool read provenance for new movements, and the legacy public writeoff POST is fail-closed disabled.

## Latest GREEN migration state

Checkpoints 118–126 establish:

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
```

Production operator path:

```text
RUN_COMPLETED (evidence only)
-> operator selects exact DRAFT/ISSUED Material Request
-> immutable session spool + ACTIVE spool + exact bridge
-> explicit POST /api/material-requests/warehouse
-> Material Request movement including direct exact spool_id for new writes
-> MaterialLedger usage tagged RWI_TX=<transaction_ref>
-> standard physical warehouse CONFIRMED writeoff
```

`MaterialRequestMovementStore` derives the new RUN_WIRE `spool_id` from immutable `JobSpoolSelection` and rejects a conflicting caller-provided spool. Historic movements without `spool_id` remain readable and continue to resolve through immutable session selection.

Public compatibility boundary:

```text
POST /api/warehouse/write-offs -> 410 legacy_writeoff_post_disabled
write_performed = false
replacement = /api/material-requests/warehouse
GET /api/warehouse/write-offs -> preserved history/coverage
```

Low-level warehouse writeoff/recovery code remains because atomic RUN_WIRE and historical startup reconciliation still require deterministic physical evidence handling.

Latest verified checkpoint-126 evidence:

```text
261e76c372e954885ee3975d845e47e608354bbc  movement spool schema
95b025271a799bcf7c175be386c33044c8c4d2b7  immutable spool derivation/serialization
e4d4e5acd5a08101ae5a6cc29943c228d822bb75  hard legacy POST boundary cleanup
21f3212d80c61ccaef2225140bfc5c5528577e47  final acceptance contract
ESP32 Build #1569   32960764524 / SUCCESS
CMP Tests #3535     32961372178 / SUCCESS
```

Checkpoint: `126_RUN_WIRE_READ_PROVENANCE_AND_LEGACY_POST_DEPRECATION_2026-08-26.md`.

## Current NEXT

1. Extend `RunWireAccountingIntegrityAudit` so a directly persisted RUN_WIRE `spool_id`, when present, must equal immutable session selection.
2. Preserve backward compatibility for historical movements without `spool_id`.
3. Keep cross-log validation bounded; do not introduce another full-log scan.
4. Continue bounded read/report provenance improvements.
5. Review internal legacy writeoff API surface without breaking recovery/history compatibility.
6. Continue software work before final hardware E2E.

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
