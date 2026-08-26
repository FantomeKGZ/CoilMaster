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
docs/PROJECT_HANDOFF/130_WAREHOUSE_DEAD_DIRECT_WRITEOFF_REMOVAL_2026-08-26.md
docs/PROJECT_HANDOFF/129_WAREHOUSE_LEGACY_SUPPORT_TYPES_NARROWING_2026-08-26.md
docs/PROJECT_HANDOFF/128_WAREHOUSE_LEGACY_DIRECT_API_NARROWING_2026-08-26.md
docs/PROJECT_HANDOFF/127_RUN_WIRE_PERSISTED_SPOOL_INTEGRITY_2026-08-26.md
docs/PROJECT_HANDOFF/126_RUN_WIRE_READ_PROVENANCE_AND_LEGACY_POST_DEPRECATION_2026-08-26.md
docs/PROJECT_HANDOFF/125_RUN_WIRE_PRICE_PROVENANCE_CONVERGENCE_2026-08-26.md
docs/PROJECT_HANDOFF/124_RUN_WIRE_CROSS_LOG_INTEGRITY_2026-08-26.md
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

Latest GREEN foundation = checkpoint **130**.

Latest verified checkpoint-130 evidence:

```text
declaration/result removal     e9ebd56a0317a7aecf87d4e6fd49e5a3433c22fd
implementation removal         2a0bde9954edbfb712336c38c677d70d405b0332
final contract alignment       6f355a7aa5b2071477b3a9bd8ac387d96abf0e13
ESP32 Build #1574              32963503796 / SUCCESS
CMP Protocol Tests #3563       32964152182 / SUCCESS
```

## Current migration state

```text
118 spool <-> MaterialLedger bridge persistence
119 authoritative wire metadata
120 explicit operator bridge creation
121 atomic RUN_WIRE transaction/recovery
122 desktop/mobile operator writeoff migrated to atomic RUN_WIRE
123 costing/finalization accounting converged: RUN_WIRE counted once
124 bounded cross-log RUN_WIRE integrity
125 one KG wire price + reserved system accounting provenance
126 direct exact spool provenance in new RUN_WIRE movements + legacy POST 410 deprecation
127 optional persisted spool_id cross-checked against immutable selection in existing bounded audit pass
128 legacy direct Store writeoff methods private-only
129 legacy direct request/result support types private-only
130 dead direct Store mutation methods/result removed; deterministic historical recovery helpers retained
```

Public `POST /api/warehouse/write-offs` remains permanently fail-closed with HTTP 410. Current production wire mutation is atomic RUN_WIRE only. Historical GET and reboot reconciliation remain compatible through retained append helpers/codecs; the obsolete direct Store mutation entrypoints no longer exist.

## Immediate NEXT

1. Continue compile-proven cleanup of warehouse helpers only where deterministic recovery/history do not depend on them.
2. Keep `appendWriteOffRecord`, `appendKgFirstWriteOffRecord`, movement codec and startup recovery while historical persisted PENDING/history can exist.
3. Prefer direct immutable transaction fields already returned by `/api/material-requests/movements`.
4. Do not add redundant full-log scans or duplicate cross-log joins.
5. Continue software optimization/integrity before final two-board hardware E2E.

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
