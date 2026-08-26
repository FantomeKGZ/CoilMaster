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

Workshop Web/CRM redesign is GREEN through Cash Web. Coordinated wire-accounting migration is GREEN through checkpoint 124: atomic RUN_WIRE has the authoritative operator UI, single-count costing semantics and bounded cross-log integrity.

## Latest GREEN migration state

Checkpoints 118–124 establish:

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

Accounting authority:

```text
wireCostMinor = confirmed physical warehouse movements
materialCostMinor = ordinary MaterialLedger usage excluding managed RWI_TX usage
totalCostMinor = wireCostMinor + materialCostMinor + labourCostMinor
```

Cross-log integrity after checkpoint 124 requires every completed atomic RUN_WIRE to resolve exactly once across Material Request movement, tagged MaterialLedger usage and tagged warehouse `KG_FIRST / SPOOL / CONFIRMED` evidence. It also revalidates immutable JobSpoolSelection plus exact spool↔warehouse-item bridge. Orphan/duplicate/mismatch fails closed.

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
- cross-log audit refuses to interpret in-flight pending/tmp;
- generic MaterialLedger Web usage cannot spoof the reserved `RWI_TX=` prefix;
- managed RUN_WIRE Ledger usage is not counted twice as generic material cost;
- standard physical warehouse CONFIRMED movement remains the wire-cost authority;
- both legacy/direct and atomic RUN_WIRE use exact `confirmedWriteOffForSourceRun(session,run)` duplicate evidence.

Latest verified checkpoint-124 evidence:

```text
9448c250955664c7e82a5e69ba26a569d3b93fe7  cross-log audit
EEF4157AC13E09D5636FAA49817BAA5A63CFC794  workshop integration
63ac31dc37f2542e3879466df9158312ac21a2f6  mandatory contracts
ESP32 Build #1560   32959482667 / SUCCESS
ESP32 Build #1561   32959521066 / SUCCESS
CMP Tests #3507     32959482741 / SUCCESS
CMP Tests #3509     32959605104 / SUCCESS
```

Checkpoint: `124_RUN_WIRE_CROSS_LOG_INTEGRITY_2026-08-26.md`.

## Current NEXT

Next software block is price/provenance convergence:

1. require atomic RUN_WIRE MaterialLedger KG-equivalent price to equal the warehouse KG price that will be persisted in physical writeoff/costing;
2. reserve system `RWI_TX=` namespace on compatibility warehouse-writeoff Web input as well as generic MaterialLedger usage;
3. extend cross-log audit to price agreement if needed;
4. expose bounded read/report provenance without creating another accounting authority;
5. only then review formal deprecation of legacy mutating POST.

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
