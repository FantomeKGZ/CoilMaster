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

Workshop Web/CRM redesign is GREEN through Cash Web. Coordinated wire-accounting migration is GREEN through checkpoint 127: atomic RUN_WIRE has authoritative operator UI, single-count costing, bounded cross-log integrity, one agreed KG price, direct exact-spool read provenance, hard-disabled legacy public writeoff POST, and persisted spool identity is verified against immutable session selection without another log pass.

## Latest GREEN migration state

Checkpoints 118–127 establish:

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

Checkpoint 127 adds no new full movement-log scan. The existing bounded movement pass detects optional persisted `spool_id`; immutable selection resolution rejects any mismatch before bridge/Ledger/warehouse evidence is trusted.

Public compatibility boundary remains:

```text
POST /api/warehouse/write-offs -> 410 legacy_writeoff_post_disabled
write_performed = false
replacement = /api/material-requests/warehouse
GET /api/warehouse/write-offs -> preserved history/coverage
```

Latest verified checkpoint-127 evidence:

```text
6965bd716ac9f4d3970bc750a8e8933b7b6fffd0  persisted spool cross-log audit
38883ae01493622d1bc98fc179fb9d9eb571ddcf  mandatory contract
ESP32 Build #1570   32961925117 / SUCCESS
CMP Tests #3541     32961999553 / SUCCESS (68/68 mandatory host steps)
```

Checkpoint: `127_RUN_WIRE_PERSISTED_SPOOL_INTEGRITY_2026-08-26.md`.

## Current NEXT

1. Review bounded read/report surfaces for places where direct transaction provenance avoids ambiguous joins.
2. Reuse existing batches/read APIs and avoid redundant full-log scans.
3. Review internal low-level legacy writeoff APIs for safe narrowing to managed/recovery-only use while preserving deterministic history/recovery.
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
