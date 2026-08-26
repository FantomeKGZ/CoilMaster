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

## GREEN foundation through checkpoint 119

```text
97-116 CRM / Material Request / Motor / Client / Cash software blocks
117 forensic exact-spool -> Material Request owner map
118 append-only spool_id <-> warehouse_item_id bridge + bounded integrity + backup/export
119 backward-compatible MaterialLedger wire metadata + exact bridge metadata validation
```

Latest verified:

```text
CMP Protocol Tests 32941574082 / SUCCESS
ESP32 Build         32941574080 / SUCCESS
```

Checkpoint 119: `119_MATERIAL_LEDGER_WIRE_METADATA_2026-08-26.md`.

## Current active queue — operator-only bridge creation

MaterialLedger now has authoritative optional wire metadata (`CU|AL` + exact diameter), while old generic records remain valid.

Next coherent block:

1. Add explicit operator-only spool-material bridge creation.
2. Require exact active physical `spool_id`.
3. Require exact active MaterialLedger `warehouse_item_id` with `unit=GRAM` and structured wire metadata.
4. Require exact CU/AL + diameter agreement between physical spool and MaterialLedger item.
5. Reject already-bridged spool fail-closed.
6. Append bridge identity evidence only; do not mutate either stock domain.
7. Keep existing exact-spool writeoff/finalization authoritative until later coordinated RUN_WIRE migration.

Future run-linked target remains:

```text
RUN_COMPLETED -> non-mutating
explicit operator warehouse ISSUE
material_request_id
source_session_id + source_run_id
CU/AL + actual consumed weight
manual confirmation
exact physical spool provenance retained through bridge
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
- cash events never mutate machine/warehouse state;
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Documentation discipline

Synchronize 95/101/06/01/90 and update 00 when read order changes. Create a numbered checkpoint with exact CI evidence for every major persistence/API/UI block.
