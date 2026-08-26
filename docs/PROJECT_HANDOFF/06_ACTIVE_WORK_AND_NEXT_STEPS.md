# Активная работа и следующие шаги

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Stable baseline

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable pre-CRM
stable-2026-08-25-pre-crm-redesign -> same commit
```

Все новые изменения только в `cmp-protocol-v1`.

## Authoritative design

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
121_RUN_WIRE_ISSUE_TRANSACTION_2026-08-26.md
122_RUN_WIRE_OPERATOR_UI_MIGRATION_2026-08-26.md
123_RUN_WIRE_ACCOUNTING_CONVERGENCE_2026-08-26.md
```

## GREEN foundation through checkpoint 123

```text
97-116 CRM / Material Request / Motor / Client / Cash software blocks
117 forensic exact-spool -> Material Request owner map
118 append-only spool_id <-> warehouse_item_id bridge + bounded integrity + backup/export
119 backward-compatible MaterialLedger wire metadata + exact bridge metadata validation
120 explicit operator-only runtime spool-material bridge creation
121 atomic explicit-operator RUN_WIRE ISSUE across Material Request + MaterialLedger + exact physical spool
122 desktop/mobile operator writeoff UI migrated to atomic RUN_WIRE ISSUE
123 RUN_WIRE costing/finalization converged to one authoritative wire-cost count
```

Latest verified checkpoint-123 evidence:

```text
29e6315c04a3901fd068df60ddc9b9849920d879  reserve RWI_TX namespace
52e0c629fe1f112ceff373b2e83decf20ff76b21  deduplicate RUN_WIRE costing
357a7677f7e91bb2a9812462e0aff8c9d0e15ea4  final semantic contract
ESP32 Build #1557   32955502232 / SUCCESS
ESP32 Build #1558   32955588907 / SUCCESS
CMP Tests #3500     32955968429 / SUCCESS
```

## Current RUN_WIRE production/accounting boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one Material Request movement
-> one MaterialLedger stock usage tagged RWI_TX=<transaction_ref>
-> one physical warehouse CONFIRMED writeoff
```

Accounting authority:

```text
wire cost = confirmed physical warehouse movement
generic material cost = ordinary MaterialLedger usage excluding managed RWI_TX usage
total = wire + generic material + labour
```

Costing and finalization fail closed while `/data/workshop/run-wire-issue.pending.json` or its temp file exists. Generic `POST /api/materials/usage` rejects the reserved `RWI_TX=` prefix, so operator-created generic material usage cannot impersonate RUN_WIRE managed accounting evidence.

Both mutation paths already query exact `confirmedWriteOffForSourceRun(source_session_id, source_run_id)` before physical deduction, giving a common exact-run duplicate boundary.

## Current active queue — cross-log integrity / legacy boundary

Next coherent block:

1. Add mandatory contract coverage proving legacy/direct and atomic RUN_WIRE share the same exact-run duplicate evidence and cannot deduct one completed run twice by changing API path.
2. Strengthen managed `RWI_TX` classification: correlate persisted Ledger usage to immutable RUN_WIRE Material Request/warehouse evidence rather than accepting a syntactically valid tag alone.
3. Keep the standard warehouse CONFIRMED movement as the only wire consumption/costing authority; do not create a second report ledger.
4. Preserve bounded read/report provenance for `material_request_id + transaction_ref + warehouse_item_id + source_session_id + source_run_id + spool_id + CU/AL + diameter + grams`.
5. After cross-log integrity is GREEN, review whether legacy mutating POST can be formally disabled while preserving GET/history/recovery compatibility.
6. Continue software optimization/integrity work before mandatory final two-board hardware E2E.

Target:

```text
one completed run
-> at most one confirmed physical writeoff
-> exactly correlated RUN_WIRE transaction evidence when atomic path used
-> one wire cost in costing/finalization
```

## Safety invariants

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts material;
- warehouse ISSUE requires explicit operator action;
- exact Material Request / warehouse item / spool / session / run provenance cannot be inferred or weakened;
- cash events never mutate machine/warehouse state;
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- backup/restore blocks on unfinished RUN_WIRE transaction;
- no automatic production-data deletion/truncation.

## Documentation discipline

Synchronize 95/101/06/01/90 and update 00 when read order changes. Create a numbered checkpoint with exact CI evidence for every major persistence/API/UI block.
