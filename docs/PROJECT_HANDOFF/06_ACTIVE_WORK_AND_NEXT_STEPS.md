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
```

## GREEN foundation through checkpoint 131

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
129 legacy direct request/result support types moved behind private API
130 obsolete direct Store mutation methods/result fully removed; historical deterministic append/recovery helpers retained
131 ambiguous repairExists(id) convenience wrapper removed; only fail-closed repairExists(id, found) remains
```

Latest verified checkpoint-131 evidence:

```text
6ccdec084001faabf25eaf5b28177d8f7e89a7d5  declaration cleanup
279fc281b42a559f89f911e9f0b2758ccd02e8ff  implementation cleanup
d848ce45eb35c7f2bba817d6de1efd0c4f4a02bd  fail-closed lookup contract
ESP32 Build #1576   32964609675 / SUCCESS
CMP Tests #3570     32964670388 / SUCCESS
CMP Tests #3571     32964882464 / SUCCESS
```

## Current RUN_WIRE production/read boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one Material Request movement (new rows carry direct exact spool_id)
-> one MaterialLedger usage tagged RWI_TX=<transaction_ref>
-> managed physical warehouse PENDING/CONFIRMED writeoff
-> one exact KG wire price
-> bounded cross-log one-to-one integrity
```

The bounded `/api/material-requests/movements` read path already exposes request/transaction/item/session/run/spool/material/diameter provenance for new RUN_WIRE rows. Historic rows without `spool_id` remain compatible through immutable selection validation.

Public mutation boundary:

```text
POST /api/warehouse/write-offs -> HTTP 410, no write
GET  /api/warehouse/write-offs -> preserved history/coverage
POST /api/material-requests/warehouse -> authoritative explicit RUN_WIRE mutation
```

Warehouse lookup boundary after checkpoint 131:

```text
repairExists(repairId)             removed
repairExists(repairId, found)      retained; bool = read/integrity success, found = identity existence
```

## Current active queue — fail-closed overload cleanup

1. Audit `loadActiveSpoolIdentity` overloads and keep only forms required by production/recovery while preserving explicit `found` where callers must distinguish missing from I/O/integrity failure.
2. Audit `loadWarehousePrice` overloads and preserve explicit `configured` semantics wherever an unset price differs from storage failure.
3. Audit `loadKnownWireDiameters` overloads and prefer success + output count rather than a count-only return that can collapse failure and zero results.
4. Compile-prove every removal with ESP32 Build and mandatory CMP contracts.
5. Then review NDJSON growth/runtime scan hot spots for bounded optimization without automatic deletion/rotation or premature DB migration.
6. Continue software optimization/integrity before mandatory final two-board hardware E2E.

Target:

```text
minimal unambiguous warehouse API
fail-closed absence/configuration semantics
historical recovery remains deterministic
bounded reports use persisted provenance
no duplicate scans
no automatic data deletion
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
