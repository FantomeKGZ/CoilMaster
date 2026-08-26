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
docs/PROJECT_HANDOFF/123_RUN_WIRE_ACCOUNTING_CONVERGENCE_2026-08-26.md
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

Latest GREEN foundation = checkpoint **123**.

Latest verified checkpoint-123 evidence:

```text
MaterialLedger Web       29e6315c04a3901fd068df60ddc9b9849920d879
RepairCosting fix        52e0c629fe1f112ceff373b2e83decf20ff76b21
final contract commit    357a7677f7e91bb2a9812462e0aff8c9d0e15ea4
ESP32 Build #1557        32955502232 / SUCCESS
ESP32 Build #1558        32955588907 / SUCCESS
CMP Protocol Tests #3500 32955968429 / SUCCESS
```

## Current migration state

```text
118 spool <-> MaterialLedger bridge persistence
119 authoritative wire metadata
120 explicit operator bridge creation
121 atomic RUN_WIRE transaction/recovery
122 desktop/mobile operator writeoff migrated to atomic RUN_WIRE
123 costing/finalization accounting converged: RUN_WIRE is counted once
```

Production operator route remains:

```text
RUN_COMPLETED (read-only evidence)
-> exact immutable spool + exact bridge
-> explicit DRAFT/ISSUED Material Request
-> explicit atomic RUN_WIRE ISSUE
-> one Material Request movement
-> one MaterialLedger stock usage tagged RWI_TX
-> one confirmed physical warehouse writeoff
```

Accounting authority after checkpoint 123:

```text
wire cost = confirmed physical warehouse movement
generic material cost = ordinary MaterialLedger usage excluding system RUN_WIRE usage
total = wire + generic materials + labour
```

Costing/finalization fails closed while RUN_WIRE pending/tmp recovery intent exists. Generic `/api/materials/usage` cannot create the reserved `RWI_TX=` prefix.

## Immediate NEXT

1. Add explicit cross-path contract coverage proving legacy/direct and atomic RUN_WIRE both reject an already-confirmed exact source run.
2. Strengthen cross-log integrity for `RWI_TX` evidence so managed Ledger usage is accepted only with matching immutable RUN_WIRE transaction evidence.
3. Keep read/report provenance bounded and avoid a second accounting authority.
4. Review safe formal deprecation boundary for legacy mutating writeoff POST while preserving historical GET/read compatibility.
5. Continue software work before final two-board hardware E2E.

## Material safety

`RUN_COMPLETED` never automatically deducts material. Warehouse mutation requires explicit operator action and exact Material Request / spool / session / run provenance.

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
