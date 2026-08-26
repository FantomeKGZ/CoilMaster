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
124_RUN_WIRE_CROSS_LOG_INTEGRITY_2026-08-26.md
125_RUN_WIRE_PRICE_PROVENANCE_CONVERGENCE_2026-08-26.md
```

## GREEN foundation through checkpoint 125

```text
97-116 CRM / Material Request / Motor / Client / Cash software blocks
117 forensic exact-spool -> Material Request owner map
118 append-only spool_id <-> warehouse_item_id bridge + bounded integrity + backup/export
119 backward-compatible MaterialLedger wire metadata + exact bridge metadata validation
120 explicit operator-only runtime spool-material bridge creation
121 atomic explicit-operator RUN_WIRE ISSUE across Material Request + MaterialLedger + exact physical spool
122 desktop/mobile operator writeoff UI migrated to atomic RUN_WIRE ISSUE
123 RUN_WIRE costing/finalization converged to one authoritative wire-cost count
124 bounded cross-log integrity for completed RUN_WIRE accounting evidence
125 one KG wire price + reserved system RWI_TX provenance
```

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

## Current RUN_WIRE accounting boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one Material Request movement
-> one MaterialLedger stock usage tagged RWI_TX=<transaction_ref>
-> one physical warehouse CONFIRMED writeoff
-> one exact KG wire price in all three accounting views
-> cross-log integrity requires exact one-to-one correlation
```

The standard warehouse CONFIRMED movement remains the single wire-cost authority for costing/finalization. MaterialLedger stock evidence and Material Request movement must carry the mathematically equivalent price, but are not counted a second time.

## Current active queue — legacy mutation deprecation / report provenance

Next coherent block:

1. Search all current code and tests for callers of mutating `POST /api/warehouse/write-offs`.
2. Verify the shared desktop/mobile production controller has no such POST and that atomic `/api/material-requests/warehouse` is the only production operator mutation path.
3. Separate public Web mutation from store-level compatibility/recovery code: historical GET, append-only records, startup reconciliation and fault tests must remain intact.
4. If no production caller remains, disable or explicitly reject the generic mutating POST instead of deleting warehouse history/recovery infrastructure.
5. Update mandatory contracts so production cannot regress to direct writeoff mutation.
6. Continue bounded read/report provenance work for Material Request transaction identity where useful.
7. Continue software optimization/integrity before mandatory final two-board hardware E2E.

Target:

```text
production operator mutation = atomic RUN_WIRE only
legacy warehouse GET/history = preserved
legacy store/recovery evidence = preserved
generic legacy POST = no production use; formal deprecation only after audited proof
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
