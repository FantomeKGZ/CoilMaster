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
130_WAREHOUSE_DEAD_DIRECT_WRITEOFF_REMOVAL_2026-08-26.md
131_WAREHOUSE_FAIL_CLOSED_REPAIR_LOOKUP_2026-08-26.md
132_WAREHOUSE_FAIL_CLOSED_SPOOL_AND_DIAMETER_LOOKUPS_2026-08-26.md
```

## GREEN foundation through checkpoint 132

```text
97-116 CRM / Material Request / Motor / Client / Cash
117-120 exact-spool -> MaterialLedger bridge + metadata + operator bridge
121-127 atomic RUN_WIRE + UI + costing + cross-log integrity + persisted spool provenance
128-130 legacy direct writeoff API/types/implementations narrowed then removed
131 repair lookup convenience wrapper removed
132 spool identity + count-only wire catalogue convenience wrappers removed
```

Latest verified checkpoint-132 evidence:

```text
6f0cae00fed57c8e90b6aa977c9658de90dc3070  declarations narrowed
237dbe93c299ea025f5199266bf78df12960a002  spool identity wrapper removed
60f7a7fa6fbaddc947bb8eb65542ee525108bf32  count-only catalogue wrapper removed
7eba027f97ad03b9e37609fd6fa1e07acec257f8  fail-closed contract coverage
ESP32 Build #1579   32966286119 / SUCCESS
CMP Tests #3578     32966344439 / SUCCESS
```

## Current production/read boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one Material Request movement
-> one MaterialLedger usage RWI_TX=<transaction_ref>
-> managed physical warehouse PENDING/CONFIRMED
```

Fail-closed warehouse lookup APIs now preserve separate result channels:

```text
repairExists(repairId, found)
loadActiveSpoolIdentity(spoolId, identity, found)
loadKnownWireDiameters(type, items, capacity, count)
loadWarehousePrice(price, configured)
```

## Current active queue

1. Move the unused one-argument `loadWarehousePrice(price)` overload out of public API; keep the explicit `configured` form authoritative.
2. Do not perform a large unrelated rewrite of `CM_WarehouseStore.cpp` merely to delete a tiny private wrapper; remove it only when the file is safely touched or can be exact-rewritten.
3. Review NDJSON growth/runtime scan hot spots and remove duplicate passes where possible while keeping fail-closed integrity.
4. No automatic rotation/deletion/truncation and no premature DB migration.
5. Continue software optimization/integrity before mandatory final two-board hardware E2E.

## Safety invariants

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout never proves Arduino idle;
- `RUN_COMPLETED` never automatically deducts material;
- exact Material Request / item / spool / session / run provenance remains mandatory;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.
