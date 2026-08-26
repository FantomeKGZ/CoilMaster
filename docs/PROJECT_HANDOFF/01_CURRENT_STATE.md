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

Workshop Web/CRM redesign is GREEN through Cash Web. Coordinated wire-accounting migration is GREEN through checkpoint 125: atomic RUN_WIRE has authoritative operator UI, single-count costing, bounded cross-log integrity and one agreed KG wire price.

## Latest GREEN migration state

Checkpoints 118–125 establish:

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

Price authority after checkpoint 125:

```text
MaterialLedger price_per_unit_minor (GRAM) * 1000
= RUN_WIRE Material Request unit_cost_minor (KG)
= warehouse price_per_kg_minor
```

The equality is enforced before the high-level pending is saved, rechecked before physical phases/recovery, and validated in completed cross-log history. Currency must agree as well.

`RWI_TX=` is system-owned. Generic `/api/materials/usage` and compatibility `/api/warehouse/write-offs` operator comments cannot create this prefix.

Latest verified checkpoint-125 evidence:

```text
74a92901262a060b203ea1b1a3cc3313537ce51a  coordinator price convergence
84dabb9920cf60ca8cd8745b16a3e97e9093f50b  persisted price audit
4f20cc928723a0c3dd873741260ebed07d8690f5  warehouse RWI_TX guard
c799c74f3f8f4c6cbcd538ea662e3c86fe039304  mandatory contracts
ESP32 Build #1562   32960004843 / SUCCESS
ESP32 Build #1563   32960173338 / SUCCESS
ESP32 Build #1564   32960269882 / SUCCESS
CMP Tests #3514     32960004874 / SUCCESS
CMP Tests #3515     32960173324 / SUCCESS
CMP Tests #3516     32960270010 / SUCCESS
CMP Tests #3517     32960329745 / SUCCESS
```

Checkpoint: `125_RUN_WIRE_PRICE_PROVENANCE_CONVERGENCE_2026-08-26.md`.

## Current NEXT

Audit the compatibility mutating warehouse Web route:

1. find every current caller of `POST /api/warehouse/write-offs`;
2. distinguish production callers from tests/history/fault-recovery infrastructure;
3. if no production caller remains, disable the public mutating POST while keeping GET/history and internal store recovery intact;
4. preserve exact-run duplicate evidence and existing immutable historical records;
5. continue bounded read/report provenance work before final hardware E2E.

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
