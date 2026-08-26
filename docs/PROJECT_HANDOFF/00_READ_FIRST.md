# CoilMaster — current project entrypoint

Дата обновления: **2026-08-26**  
Repo: `FantomeKGZ/CoilMaster`  
Source-of-truth: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Stable pre-CRM snapshot

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> same commit
stable-2026-08-25-pre-crm-redesign -> same commit
```

Вся новая разработка только в `cmp-protocol-v1`.

## Read order

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/122_RUN_WIRE_OPERATOR_UI_MIGRATION_2026-08-26.md
docs/PROJECT_HANDOFF/121_RUN_WIRE_ISSUE_TRANSACTION_2026-08-26.md
docs/PROJECT_HANDOFF/120_OPERATOR_SPOOL_MATERIAL_BRIDGE_WEB_2026-08-26.md
docs/PROJECT_HANDOFF/119_MATERIAL_LEDGER_WIRE_METADATA_2026-08-26.md
docs/PROJECT_HANDOFF/118_SPOOL_MATERIAL_BRIDGE_PERSISTENCE_2026-08-26.md
docs/PROJECT_HANDOFF/117_SPOOL_TO_MATERIAL_REQUEST_MIGRATION_MAP_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

Latest GREEN foundation = checkpoint **122**.

Latest verified checkpoint-122 evidence:

```text
operator UI commit       5c28fadd4a3d1ef8de272f677e2b2f53bfc77794
UI contract commit       10528b23336bebe30208a56e085d3d77aeb19af9
fault-contract fix       f8d25c1b5fb04bddbd0c2b93fca704f14a7b565f
ESP32 Build #1555        32954324723 / SUCCESS
ESP32 Build #1556        32954467677 / SUCCESS
CMP Protocol Tests #3489 32954794059 / SUCCESS
Reference #101/#102      32954324914 / 32954467670 / SUCCESS
```

## Current migration state

The exact physical spool ↔ MaterialLedger migration now has both a crash-safe transaction and the authoritative operator Web path:

```text
118 persistence + bounded integrity + backup/export
119 authoritative backward-compatible MaterialLedger wire metadata
120 explicit operator-only spool-material bridge creation
121 explicit operator-only atomic RUN_WIRE ISSUE
122 desktop/mobile operator writeoff migrated to atomic RUN_WIRE
```

Production operator route:

```text
RUN_COMPLETED (read-only evidence)
-> exact immutable spool + exact bridge
-> explicit DRAFT/ISSUED Material Request selection
-> POST /api/material-requests/warehouse
   confirmed=true
   movement_kind=ISSUE
   source_kind=RUN_WIRE
   unit=KG
   material_request_id
   warehouse_item_id
   source_session_id + source_run_id
   exact spool_id
   CU|AL + exact diameter
   actual consumed grams
```

The production shared controller no longer performs mutating POST to `/api/warehouse/write-offs`. That endpoint remains compatibility/history/fault-recovery infrastructure. Existing strict costing/finalization still consumes the standard confirmed physical writeoff evidence emitted by the atomic transaction.

## Immediate NEXT

1. Audit costing/finalization/report consumers for double-accounting between MaterialLedger RUN_WIRE usage and standard physical writeoff evidence.
2. Verify/expose exact `material_request_id + transaction_ref + warehouse_item_id + source_session_id + source_run_id + spool_id` provenance in read/report surfaces.
3. Add explicit duplicate/double-accounting contract coverage between compatibility direct writeoff and atomic RUN_WIRE.
4. Review safe formal deprecation boundary for legacy mutating POST without removing historical read compatibility.
5. Continue software work before the final two-board hardware E2E.

## Material safety

`RUN_COMPLETED` never automatically deducts material. Warehouse mutation requires explicit operator action. RUN_WIRE requires exact Material Request, physical spool/run provenance, bridge identity and a crash-safe high-level pending owner.

## General safety invariants

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- backup/restore is blocked while RUN_WIRE recovery intent exists;
- no automatic production deletion/truncation.

## Working discipline

Before modifying an existing file: fetch exact `cmp-protocol-v1` content + current blob SHA. Before a new path: confirm 404. Never claim GREEN without actual CI/build evidence.
