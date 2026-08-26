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
docs/PROJECT_HANDOFF/118_SPOOL_MATERIAL_BRIDGE_PERSISTENCE_2026-08-26.md
docs/PROJECT_HANDOFF/117_SPOOL_TO_MATERIAL_REQUEST_MIGRATION_MAP_2026-08-26.md
docs/PROJECT_HANDOFF/116_CASH_WEB_UI_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

Latest GREEN foundation = checkpoint **118**.

Latest verified:

```text
CMP Protocol Tests 32939884633 / SUCCESS
ESP32 Build         32939884635 / SUCCESS
```

## Current migration state

A persistent append-only bridge now exists between exact physical spools and generic MaterialLedger items:

```text
spool_id <-> warehouse_item_id + CU/AL + diameter
/data/warehouse/spool-material-bridges.ndjson
```

It has bounded integrity checks and warehouse backup/export coverage. There is intentionally no runtime bridge-creation API yet. Existing exact-spool writeoff/finalization remains authoritative.

## Immediate NEXT

1. Extend authoritative `MaterialLedger` backward-compatibly with structured wire metadata (`CU|AL` + exact diameter).
2. Preserve current generic material, unit, stock and costing semantics for all existing records.
3. Require exact physical-spool ↔ MaterialLedger wire-metadata agreement before any bridge creation.
4. Expose bridge creation only as explicit operator action after the metadata contract is GREEN.
5. Then migrate run-linked wire ISSUE transactionally to Material Request while preserving exact `source_session_id + source_run_id` and physical spool provenance.

## Material safety

`RUN_COMPLETED` never automatically deducts material. Warehouse mutation requires explicit operator action. Current exact `spool_id` production contract remains authoritative until coordinated runtime migration across job/writeoff/request/costing/finalization/backup/integrity/reports/Web/tests is complete.

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
