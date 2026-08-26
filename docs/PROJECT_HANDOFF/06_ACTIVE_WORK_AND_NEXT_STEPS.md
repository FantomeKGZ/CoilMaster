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
```

## GREEN foundation through checkpoint 127

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
```

Latest verified checkpoint-127 evidence:

```text
6965bd716ac9f4d3970bc750a8e8933b7b6fffd0  persisted spool cross-log audit
38883ae01493622d1bc98fc179fb9d9eb571ddcf  mandatory contract
ESP32 Build #1570   32961925117 / SUCCESS
CMP Tests #3541     32961999553 / SUCCESS (68/68 mandatory host steps)
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

New movement `spool_id` is derived from immutable session selection at write time and independently cross-checked against the same immutable selection during bounded accounting integrity. Historic RUN_WIRE movements without the field remain valid and continue using immutable selection as authority.

Public mutation boundary remains:

```text
POST /api/warehouse/write-offs -> HTTP 410, no write
GET  /api/warehouse/write-offs -> preserved history/coverage
POST /api/material-requests/warehouse -> authoritative explicit RUN_WIRE mutation
```

## Current active queue — bounded reports / internal API narrowing

Next coherent block:

1. Audit read/report surfaces for any remaining ambiguous reconstruction of RUN_WIRE transaction identity.
2. Prefer already persisted `material_request_id + transaction_ref + warehouse_item_id + source_session_id + source_run_id + spool_id + CU/AL + diameter` instead of new joins where available.
3. Reuse existing bounded stores/cross-log batches; do not add another full-log scan.
4. Inspect callers of low-level `WarehouseStore::confirmSpoolWriteOff` and `confirmKgFirstWriteOff` now that public legacy POST is disabled.
5. If only managed RUN_WIRE/recovery paths remain, narrow low-level API visibility/semantics without breaking deterministic startup recovery or historical GET.
6. Continue software optimization/integrity before mandatory final two-board hardware E2E.

Target:

```text
bounded reports use direct immutable provenance where available
no ambiguous transaction joins
no extra full-log scan
public legacy POST stays disabled
low-level compatibility surface no broader than required by managed/recovery code
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
