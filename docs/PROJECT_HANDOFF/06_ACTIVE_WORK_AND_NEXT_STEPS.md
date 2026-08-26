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
```

## GREEN foundation through checkpoint 124

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
```

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

## Current RUN_WIRE production/accounting boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one Material Request movement
-> one MaterialLedger stock usage tagged RWI_TX=<transaction_ref>
-> one physical warehouse CONFIRMED writeoff
-> cross-log integrity requires exact one-to-one correlation
```

Cross-log agreement includes exact request/repair/item/session/run, immutable spool, spool-material bridge, CU/AL, diameter, consumed grams, Ledger quantity, currency and timestamp. The fixed batch size is 16; there is no unbounded in-memory transaction index.

Accounting authority remains:

```text
wire cost = confirmed physical warehouse movement
generic material cost = ordinary MaterialLedger usage excluding managed RWI_TX usage
total = wire + generic material + labour
```

## Current active queue — price/provenance convergence

Next coherent block:

1. Enforce that the MaterialLedger wire item's KG-equivalent price used to create the RUN_WIRE request/usage equals the standard warehouse `price_per_kg_minor` used by physical CONFIRMED writeoff and costing.
2. Fail before saving the high-level RUN_WIRE pending if the two price authorities disagree, so no partial transaction can be started with split costing semantics.
3. Reserve `RWI_TX=` on compatibility direct warehouse-writeoff Web comments; only the managed coordinator may create system accounting provenance.
4. Extend mandatory contracts and cross-log audit to prove price equality where the persisted schemas allow it.
5. Preserve bounded read/report provenance and standard warehouse CONFIRMED as the single wire-cost authority.
6. After this is GREEN, review the formal deprecation boundary for legacy mutating POST while preserving GET/history/recovery compatibility.
7. Continue software optimization/integrity work before mandatory final two-board hardware E2E.

Target:

```text
one completed atomic RUN_WIRE
-> one request movement
-> one Ledger usage
-> one physical CONFIRMED writeoff
-> one exact spool/run/material identity
-> one agreed KG wire price
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
