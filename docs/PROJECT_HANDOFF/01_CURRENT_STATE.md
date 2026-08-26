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

Authoritative design:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

## Latest GREEN migration state

Checkpoint 117 mapped the current exact-spool owners and found two independent warehouse identities:

```text
physical spool domain -> spool_id
MaterialLedger domain -> warehouse_item_id
```

Checkpoint 118 added the safe persistence bridge:

```text
spool_id <-> warehouse_item_id + CU/AL + diameter
/data/warehouse/spool-material-bridges.ndjson
```

Properties:
- append-only;
- duplicate/conflicting spool mapping fails closed;
- bounded duplicate audit (24 IDs/batch);
- cross-reference audit requires exact physical spool match and exact MaterialLedger item;
- wire-compatible MaterialLedger unit currently must be `GRAM`;
- bridge is covered by warehouse backup/export/integrity;
- no runtime bridge-creation API is exposed yet.

Latest evidence:

```text
CMP Protocol Tests 32939884633 / SUCCESS
ESP32 Build         32939884635 / SUCCESS
```

## Current NEXT

Extend the existing authoritative MaterialLedger backward-compatibly with structured wire metadata (`CU|AL` + exact diameter). Preserve all existing non-wire catalog records and costing/unit contracts. Only after authoritative metadata exists may an explicit operator bridge-creation flow be exposed.

Then migrate run-linked wire accounting coherently toward:

```text
RUN_COMPLETED -> never auto-deducts
operator confirms warehouse ISSUE
material_request_id
exact source_session_id + source_run_id
CU/AL + actual consumed weight
exact physical spool provenance retained through bridge
```

Current exact `spool_id` backend/finalization/writeoff checks remain authoritative until the runtime transition across job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests is complete.

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
