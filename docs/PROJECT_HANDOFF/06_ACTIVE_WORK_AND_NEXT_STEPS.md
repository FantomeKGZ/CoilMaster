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
```

## GREEN foundation through checkpoint 121

```text
97-116 CRM / Material Request / Motor / Client / Cash software blocks
117 forensic exact-spool -> Material Request owner map
118 append-only spool_id <-> warehouse_item_id bridge + bounded integrity + backup/export
119 backward-compatible MaterialLedger wire metadata + exact bridge metadata validation
120 explicit operator-only runtime spool-material bridge creation
121 atomic explicit-operator RUN_WIRE ISSUE across Material Request + MaterialLedger + exact physical spool
```

Latest verified source/test evidence:

```text
source commit        db643d33cd5327556429e71f3734864c484d2f40
final test commit    7e73e9016c690e3ec65dfacfe3a80328b05a2148
ESP32 Build #1551    32951550134 / SUCCESS
CMP Tests #3475      32951582879 / SUCCESS
```

Checkpoint 121: `121_RUN_WIRE_ISSUE_TRANSACTION_2026-08-26.md`.

## Checkpoint 121 transaction boundary

RUN_WIRE is no longer allowed through the generic Ledger-only path. The explicit operator route requires:

```text
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
```

One durable high-level pending freezes exact identity, before/after spool weight and costing. Recovery verifies Material Request movement, Ledger `RWI_TX` usage, standard warehouse CONFIRMED writeoff and exact spool state. Impossible ordering fails closed.

Backup/restore is blocked while RUN_WIRE pending/tmp recovery intent exists.

## Current active queue — operator/report completion

Next coherent block:

1. Audit current Web operator surfaces that can perform or suggest exact-spool writeoff and Material Request warehouse ISSUE.
2. Add a bounded operator path for the new atomic RUN_WIRE ISSUE without any automatic action on `RUN_COMPLETED`.
3. Show/retain exact `material_request_id + source_session_id + source_run_id + spool_id + warehouse_item_id` provenance in read/report surfaces.
4. Verify costing/finalization/report consumers continue to use standard confirmed physical writeoff evidence and do not double-count MaterialLedger usage.
5. Add contract tests for duplicate/double-accounting prevention across old direct exact-spool and new RUN_WIRE paths.
6. Keep the legacy direct exact-spool writeoff available until this operator/report transition is fully GREEN.
7. Continue software optimization/integrity work before mandatory final two-board hardware E2E.

Target:

```text
RUN_COMPLETED -> non-mutating
operator reviews completed run
operator chooses exact Material Request / exact bridged spool
explicit atomic RUN_WIRE ISSUE
one physical confirmed writeoff evidence
one MaterialLedger usage evidence
one Material Request movement evidence
reports/costing/finalization remain consistent
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
- exact spool/session/run provenance cannot be inferred or weakened;
- cash events never mutate machine/warehouse state;
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- backup/restore blocks on unfinished RUN_WIRE transaction;
- no automatic production-data deletion/truncation.

## Documentation discipline

Synchronize 95/101/06/01/90 and update 00 when read order changes. Create a numbered checkpoint with exact CI evidence for every major persistence/API/UI block.
