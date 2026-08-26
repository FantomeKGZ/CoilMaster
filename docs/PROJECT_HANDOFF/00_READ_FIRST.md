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
docs/PROJECT_HANDOFF/120_OPERATOR_SPOOL_MATERIAL_BRIDGE_WEB_2026-08-26.md
docs/PROJECT_HANDOFF/119_MATERIAL_LEDGER_WIRE_METADATA_2026-08-26.md
docs/PROJECT_HANDOFF/118_SPOOL_MATERIAL_BRIDGE_PERSISTENCE_2026-08-26.md
docs/PROJECT_HANDOFF/117_SPOOL_TO_MATERIAL_REQUEST_MIGRATION_MAP_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

Latest GREEN foundation = checkpoint **120**.

Latest verified on final checkpoint-120 tree `fa651e3e50a25df9489db24b6c71bd853171a9b8`:

```text
CMP Protocol Tests 32944119683 / SUCCESS
ESP32 Build         32944119688 / SUCCESS
```

## Current migration state

The exact physical spool ↔ MaterialLedger bridge now has all three required layers:

```text
118 persistence + bounded integrity + backup/export
119 authoritative backward-compatible MaterialLedger wire metadata
120 explicit operator-only runtime bridge creation
```

Production route:

```text
POST /api/warehouse/spool-material-bridges
```

Creation requires explicit confirmation and exact ACTIVE physical spool ↔ ACTIVE MaterialLedger `GRAM + CU/AL + diameter` agreement. It appends identity evidence only and does not mutate stock.

Existing exact-spool writeoff/finalization remains authoritative.

## Immediate NEXT

1. Inspect existing MaterialLedger usage/adjustment transaction behavior and current Material Request warehouse coordinator.
2. Define one crash-safe pending/recovery boundary for explicit operator run-linked wire ISSUE.
3. Require exact `material_request_id + source_session_id + source_run_id`.
4. Preserve exact physical `spool_id` provenance through the bridge plus CU/AL, diameter and actual consumed weight.
5. Keep `RUN_COMPLETED` non-mutating.
6. Do not retire old exact-spool production requirements until movement/costing/finalization/backup/integrity/reports/Web/tests migrate coherently and verify GREEN.

## Material safety

`RUN_COMPLETED` never automatically deducts material. Warehouse mutation requires explicit operator action. Current exact `spool_id` production contract remains authoritative until coordinated runtime migration is complete.

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
- no automatic production deletion/truncation.

## Working discipline

Before modifying an existing file: fetch exact `cmp-protocol-v1` content + current blob SHA. Before a new path: confirm 404. Never claim GREEN without actual CI/build evidence.
