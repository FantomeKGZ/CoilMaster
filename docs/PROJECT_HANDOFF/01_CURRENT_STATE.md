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

Workshop Web/CRM redesign is GREEN through Cash Web. Coordinated wire-accounting migration is GREEN through checkpoint 123: atomic RUN_WIRE has the authoritative operator UI and converged single-count costing/finalization semantics.

## Latest GREEN migration state

Checkpoints 118–123 establish:

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
```

Production operator path:

```text
RUN_COMPLETED (evidence only)
-> operator selects exact DRAFT/ISSUED Material Request
-> immutable session spool + ACTIVE spool + exact bridge
-> explicit POST /api/material-requests/warehouse
-> Material Request movement
-> MaterialLedger usage tagged RWI_TX=<transaction_ref>
-> standard physical warehouse CONFIRMED writeoff
```

Accounting after checkpoint 123:

```text
wireCostMinor = confirmed physical warehouse movements
materialCostMinor = ordinary MaterialLedger usage excluding managed RWI_TX usage
totalCostMinor = wireCostMinor + materialCostMinor + labourCostMinor
```

Properties:
- `RUN_COMPLETED` remains strictly non-mutating;
- explicit operator confirmation remains mandatory;
- exact Material Request ↔ repair ownership is validated;
- exact immutable `JobSpoolSelection` is required;
- exact `source_session_id + source_run_id` must be Completed;
- exact spool-material bridge is required;
- ACTIVE physical spool and ACTIVE MaterialLedger wire item must agree on CU/AL + diameter;
- one `/data/workshop/run-wire-issue.pending.json` owns cross-store recovery;
- impossible ordering or unknown spool weight fails closed;
- backup/restore is Busy while RUN_WIRE pending/tmp exists;
- costing/finalization also fail closed while RUN_WIRE pending/tmp exists;
- generic MaterialLedger Web usage cannot spoof the reserved `RWI_TX=` prefix;
- managed RUN_WIRE Ledger usage is not counted a second time as generic material cost;
- standard physical warehouse CONFIRMED movement remains the wire-cost authority;
- legacy writeoff API remains compatibility/history/fault-recovery infrastructure;
- both legacy/direct and atomic RUN_WIRE use exact `confirmedWriteOffForSourceRun(session,run)` duplicate evidence.

Latest verified checkpoint-123 evidence:

```text
29e6315c04a3901fd068df60ddc9b9849920d879  reserve RWI_TX namespace
52e0c629fe1f112ceff373b2e83decf20ff76b21  deduplicate RUN_WIRE costing
357a7677f7e91bb2a9812462e0aff8c9d0e15ea4  final semantic contract
ESP32 Build #1557   32955502232 / SUCCESS
ESP32 Build #1558   32955588907 / SUCCESS
CMP Tests #3500     32955968429 / SUCCESS
```

Checkpoint: `123_RUN_WIRE_ACCOUNTING_CONVERGENCE_2026-08-26.md`.

## Current NEXT

Strengthen cross-path and cross-log integrity without creating a second accounting authority.

Next software block:

1. contract-prove symmetric exact-run duplicate protection between compatibility direct writeoff and atomic RUN_WIRE;
2. require persisted managed `RWI_TX` usage to correlate to immutable RUN_WIRE transaction evidence rather than trusting the tag alone;
3. keep standard confirmed physical writeoff as the single consumption/costing authority;
4. retain bounded read/report provenance for `material_request_id + transaction_ref + warehouse_item_id + source_session_id + source_run_id + spool_id`;
5. define safe deprecation boundary for legacy mutating POST only after the cross-log audit is GREEN.

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
- cash operations never trigger machine/warehouse mutation;
- run-linked wire movement preserves exact run + spool provenance;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- backup/restore cannot cross unfinished RUN_WIRE recovery;
- no automatic production-data deletion/truncation.

## Hardware acceptance

Not complete. Full two-board E2E remains mandatory after CRM/material/writeoff/report contracts stabilize.
