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
```

## GREEN foundation through checkpoint 120

```text
97-116 CRM / Material Request / Motor / Client / Cash software blocks
117 forensic exact-spool -> Material Request owner map
118 append-only spool_id <-> warehouse_item_id bridge + bounded integrity + backup/export
119 backward-compatible MaterialLedger wire metadata + exact bridge metadata validation
120 explicit operator-only runtime spool-material bridge creation
```

Latest verified on final checkpoint-120 tree `fa651e3e50a25df9489db24b6c71bd853171a9b8`:

```text
CMP Protocol Tests 32944119683 / SUCCESS
ESP32 Build         32944119688 / SUCCESS
```

Checkpoint 120: `120_OPERATOR_SPOOL_MATERIAL_BRIDGE_WEB_2026-08-26.md`.

## Current active queue — crash-safe run-linked RUN_WIRE migration

The physical spool ↔ MaterialLedger identity bridge is now available through an explicit operator-only POST. Bridge creation appends identity evidence only and does not mutate either stock domain.

Next coherent block:

1. Inspect existing `MaterialLedger` usage/adjustment transaction semantics and current Material Request warehouse coordinator before choosing the mutation owner.
2. Design one crash-safe explicit-operator transaction for run-linked wire ISSUE; do not perform two independent writes without pending/recovery evidence.
3. Require exact `material_request_id + source_session_id + source_run_id`.
4. Preserve exact physical `spool_id` provenance through the authoritative spool-material bridge.
5. Preserve exact `CU|AL + diameter` identity and actual consumed weight.
6. Keep `RUN_COMPLETED` strictly non-mutating.
7. Integrate movement/costing/finalization/backup/integrity/reports/Web/tests coherently before retiring any old exact-spool production requirement.

Target:

```text
RUN_COMPLETED -> non-mutating
explicit operator warehouse ISSUE
material_request_id
source_session_id + source_run_id
exact physical spool provenance via bridge
CU/AL + actual consumed weight
manual confirmation
crash-safe pending/recovery
```

Current exact-spool writeoff/finalization remains authoritative until this coordinated transition is fully GREEN end-to-end.

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
- cash events never mutate machine/warehouse state;
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Documentation discipline

Synchronize 95/101/06/01/90 and update 00 when read order changes. Create a numbered checkpoint with exact CI evidence for every major persistence/API/UI block.
