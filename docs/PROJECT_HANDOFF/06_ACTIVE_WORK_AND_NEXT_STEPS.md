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
```

## GREEN foundation through checkpoint 122

```text
97-116 CRM / Material Request / Motor / Client / Cash software blocks
117 forensic exact-spool -> Material Request owner map
118 append-only spool_id <-> warehouse_item_id bridge + bounded integrity + backup/export
119 backward-compatible MaterialLedger wire metadata + exact bridge metadata validation
120 explicit operator-only runtime spool-material bridge creation
121 atomic explicit-operator RUN_WIRE ISSUE across Material Request + MaterialLedger + exact physical spool
122 desktop/mobile operator writeoff UI migrated to atomic RUN_WIRE ISSUE
```

Latest verified checkpoint-122 source/test evidence:

```text
operator UI commit       5c28fadd4a3d1ef8de272f677e2b2f53bfc77794
UI contract commit       10528b23336bebe30208a56e085d3d77aeb19af9
fault-contract fix       f8d25c1b5fb04bddbd0c2b93fca704f14a7b565f
CMP Protocol Tests #3489 32954794059 / SUCCESS
```

Checkpoint 122: `122_RUN_WIRE_OPERATOR_UI_MIGRATION_2026-08-26.md`.

## Current RUN_WIRE production boundary

`RUN_COMPLETED` remains strictly non-mutating. Manual wire consumption now follows one operator path:

```text
operator reviews exact uncovered RUN_COMPLETED
-> immutable source session/run spool
-> exact spool <-> MaterialLedger bridge
-> explicit DRAFT/ISSUED Material Request selection
-> actual consumed kg
-> POST /api/material-requests/warehouse
   confirmed=true
   movement_kind=ISSUE
   source_kind=RUN_WIRE
   unit=KG
   material_request_id
   repair_id
   warehouse_item_id
   source_session_id + source_run_id
   exact spool_id
   CU|AL + exact diameter
   actual consumed grams
-> one durable RunWireIssueCoordinator transaction/recovery owner
```

The active production UI no longer performs a mutating POST to `/api/warehouse/write-offs`. Legacy warehouse writeoff GET remains authoritative read/history/coverage evidence; the old backend POST stays compatibility-only until formal deprecation is safe.

## Current active queue — accounting/report convergence

Next coherent block:

1. Audit costing, finalization and report consumers for double-accounting between MaterialLedger RUN_WIRE usage and standard confirmed physical writeoff evidence.
2. Keep standard confirmed physical spool writeoff as the one consumption evidence used by costing/finalization unless an explicit audited migration says otherwise.
3. Expose/retain exact `material_request_id + warehouse_item_id + source_session_id + source_run_id + spool_id` provenance in operator/report surfaces where needed.
4. Add contract coverage that rejects double-counting and rejects any reintroduction of direct legacy mutation from production UI.
5. Review formal deprecation boundary for legacy direct writeoff POST while preserving historical read compatibility.
6. Continue software optimization/integrity work before mandatory final two-board hardware E2E.

Target:

```text
RUN_COMPLETED -> non-mutating
explicit operator RUN_WIRE ISSUE
one Material Request movement
one MaterialLedger usage
one physical confirmed writeoff evidence
costing/finalization/reporting count consumption once
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
