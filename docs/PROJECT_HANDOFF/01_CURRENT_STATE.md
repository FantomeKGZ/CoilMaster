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

GREEN through checkpoint 132. Atomic RUN_WIRE is the only current wire mutation path; accounting/provenance is converged and bounded; obsolete direct warehouse mutation code is deleted; repair/spool/catalogue lookups now keep I/O success separate from `found/count`.

## Latest GREEN migration state

Checkpoints 118–132 establish:

```text
explicit operator RUN_WIRE ISSUE
exact material_request_id + warehouse_item_id + spool_id
source_session_id + source_run_id
one durable high-level recovery owner
one authoritative wire-cost count
bounded cross-log exact transaction integrity
legacy public writeoff POST = HTTP 410
historical deterministic recovery retained
ambiguous repairExists(id) wrapper removed
ambiguous loadActiveSpoolIdentity(id, identity) wrapper removed
count-only loadKnownWireDiameters(...) wrapper removed
fail-closed found/count forms retained
```

Production path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable Material Request movement
-> MaterialLedger usage tagged RWI_TX=<transaction_ref>
-> managed physical warehouse PENDING/CONFIRMED evidence
```

Latest verified checkpoint-132 evidence:

```text
6f0cae00fed57c8e90b6aa977c9658de90dc3070  declarations narrowed
237dbe93c299ea025f5199266bf78df12960a002  spool identity wrapper removed
60f7a7fa6fbaddc947bb8eb65542ee525108bf32  count-only catalogue wrapper removed
7eba027f97ad03b9e37609fd6fa1e07acec257f8  fail-closed contract coverage
ESP32 Build #1579   32966286119 / SUCCESS
CMP Tests #3578     32966344439 / SUCCESS
```

Checkpoint: `132_WAREHOUSE_FAIL_CLOSED_SPOOL_AND_DIAMETER_LOOKUPS_2026-08-26.md`.

## Current NEXT

1. Narrow unused `loadWarehousePrice(price)` behind private; keep `loadWarehousePrice(price, configured)` public.
2. Remove the private wrapper later only via exact safe rewrite of the large store source.
3. Review NDJSON growth/runtime scan hot spots for bounded optimization without automatic deletion/rotation or premature DB migration.
4. Preserve managed RUN_WIRE, GET/history, integrity, backup and deterministic historical recovery.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains explicit/operator-only/transactional/fail-closed; no automatic production-data deletion/truncation.

## Hardware acceptance

Not complete. Full two-board E2E remains mandatory after software stabilization.
