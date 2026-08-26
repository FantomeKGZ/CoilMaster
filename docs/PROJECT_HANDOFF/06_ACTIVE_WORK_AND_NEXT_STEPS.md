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
126_RUN_WIRE_READ_PROVENANCE_AND_LEGACY_POST_DEPRECATION_2026-08-26.md
127_RUN_WIRE_PERSISTED_SPOOL_INTEGRITY_2026-08-26.md
128_WAREHOUSE_LEGACY_DIRECT_API_NARROWING_2026-08-26.md
129_WAREHOUSE_LEGACY_SUPPORT_TYPES_NARROWING_2026-08-26.md
```

## GREEN foundation through checkpoint 129

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
126 direct exact spool provenance in new immutable RUN_WIRE movement + public legacy writeoff POST hard-disabled
127 persisted spool_id cross-checked against immutable selection in existing bounded audit pass
128 legacy direct Store mutation methods moved behind private API
129 legacy direct request/result support types moved behind private API; movement read already exposes direct RUN_WIRE provenance
```

Latest verified checkpoint-129 evidence:

```text
b2f7f13f88bf2a5e489999fdf318523fc1fcdf46  legacy support type narrowing
071e55923ead09264a801c250e8807a17823eba1  private-type contract
ESP32 Build #1572   32963035385 / SUCCESS
CMP Tests #3551     32963113298 / SUCCESS
```

## Current RUN_WIRE production/read boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one Material Request movement
   new writes carry direct exact spool_id
-> one MaterialLedger usage tagged RWI_TX=<transaction_ref>
-> one physical warehouse CONFIRMED writeoff
-> one exact KG wire price in all accounting views
-> cross-log integrity requires exact one-to-one correlation
```

The bounded `/api/material-requests/movements` read path returns immutable movement JSON directly. New movement rows already include exact request/transaction/item/session/run/spool/material/diameter provenance; historic rows without `spool_id` remain compatible through immutable selection validation.

Public mutation boundary remains:

```text
POST /api/warehouse/write-offs -> HTTP 410, no write
GET  /api/warehouse/write-offs -> preserved history/coverage
POST /api/material-requests/warehouse -> authoritative explicit RUN_WIRE mutation
```

Internal narrowing now also means:

```text
confirmSpoolWriteOff / confirmKgFirstWriteOff -> private
ConfirmedSpoolWriteOff / SpoolWriteOffResult -> private
KgFirstWriteOff + managed RUN_WIRE methods -> public production surface
```

## Current active queue — dead helper removal / bounded provenance

Next coherent block:

1. Audit whether the two private direct mutation methods are now entirely dead and can be removed while keeping `appendWriteOffRecord` for deterministic legacy recovery.
2. Preserve `appendKgFirstWriteOffRecord`, `appendWriteOffRecord`, movement codec and startup recovery as long as historical pending reconciliation depends on them.
3. Prefer direct immutable movement provenance for reports; do not add another full-log scan or duplicate bridge join.
4. Narrow other warehouse helpers only when ESP32 compilation and mandatory contracts prove no current caller.
5. Continue software optimization/integrity before mandatory final two-board hardware E2E.

Target:

```text
no unnecessary legacy mutation code
historical recovery remains deterministic
bounded reports use persisted provenance
no redundant scans
public legacy POST remains disabled
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
