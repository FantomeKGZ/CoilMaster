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

## GREEN foundation through checkpoint 118

```text
97-116 CRM / Material Request / Motor / Client / Cash software blocks
117 forensic exact-spool -> Material Request owner map
118 append-only spool_id <-> warehouse_item_id bridge + bounded integrity + backup/export
```

Latest verified:

```text
CMP Protocol Tests 32939884633 / SUCCESS
ESP32 Build         32939884635 / SUCCESS
```

Checkpoint 118: `118_SPOOL_MATERIAL_BRIDGE_PERSISTENCE_2026-08-26.md`.

## Current active queue — wire metadata before runtime bridge creation

The bridge persistence layer exists, but no runtime writer is exposed yet.

Next coherent block:

1. Extend existing authoritative `MaterialLedger` backward-compatibly with structured wire metadata for wire catalog entries: `CU|AL` + exact diameter.
2. Preserve all non-wire material records and current unit/cost contracts.
3. Make spool bridge creation fail closed unless exact physical spool and exact MaterialLedger wire metadata agree.
4. Only then expose an explicit operator bridge-creation path.
5. After bridge identity is authoritative, migrate run-linked accounting toward Material Request `RUN_WIRE` with crash-safe transaction/recovery.

Do **not** partially remove old exact-spool writeoff/finalization requirements before the coordinated runtime transition is complete.

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
