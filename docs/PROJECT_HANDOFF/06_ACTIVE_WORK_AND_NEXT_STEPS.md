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
```

## GREEN foundation through checkpoint 126

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
```

Latest verified checkpoint-126 evidence:

```text
261e76c372e954885ee3975d845e47e608354bbc  movement spool schema
95b025271a799bcf7c175be386c33044c8c4d2b7  immutable spool derivation/serialization
e4d4e5acd5a08101ae5a6cc29943c228d822bb75  legacy POST hard boundary
21f3212d80c61ccaef2225140bfc5c5528577e47  final acceptance contract
ESP32 Build #1569   32960764524 / SUCCESS
CMP Tests #3535     32961372178 / SUCCESS
```

## Current RUN_WIRE production/read boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one Material Request movement
   including direct exact spool_id for new writes
-> one MaterialLedger usage tagged RWI_TX=<transaction_ref>
-> one physical warehouse CONFIRMED writeoff
-> one exact KG wire price in all accounting views
-> cross-log integrity requires exact one-to-one correlation
```

`MaterialRequestMovementStore` derives RUN_WIRE `spool_id` from immutable session selection and rejects mismatch. Historical RUN_WIRE movements without the new field remain valid/readable through the immutable selection.

Public mutation boundary is now explicit:

```text
POST /api/warehouse/write-offs -> HTTP 410, no write
GET  /api/warehouse/write-offs -> preserved history/coverage
POST /api/material-requests/warehouse -> authoritative explicit RUN_WIRE mutation
```

## Current active queue — direct spool integrity / bounded reports

Next coherent block:

1. Extend `RunWireAccountingIntegrityAudit` to parse optional persisted `spool_id` from RUN_WIRE Material Request movement.
2. When present, require exact equality with immutable `JobSpoolSelection.spoolId` before accepting cross-log evidence.
3. When absent, preserve historical backward compatibility and resolve spool from immutable selection exactly as today.
4. Keep fixed batching and existing log-pass count; do not add a new per-record or full-log scan.
5. Continue bounded read/report provenance improvements for Material Request transaction identity.
6. Review whether low-level legacy writeoff APIs can be narrowed internally without affecting historical GET/recovery.
7. Continue software optimization/integrity before mandatory final two-board hardware E2E.

Target:

```text
new RUN_WIRE movement spool_id == immutable session selection spool_id
historic movement without spool_id -> selection remains authority
no extra full-log scan
public legacy POST stays disabled
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
