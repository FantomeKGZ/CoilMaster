# CoilMaster — current project entrypoint

Дата обновления: **2026-08-27**  
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
docs/PROJECT_HANDOFF/143_AUTONOMOUS_ASSIGNMENT_API_NARROWING_2026-08-27.md
docs/PROJECT_HANDOFF/142_JOB_SNAPSHOT_EXISTS_VISIBILITY_2026-08-27.md
docs/PROJECT_HANDOFF/141_CRM_FAIL_CLOSED_SOURCE_LOOKUPS_2026-08-26.md
docs/PROJECT_HANDOFF/140_WINDING_SESSION_SELECTION_ONLY_PREFLIGHT_2026-08-26.md
docs/PROJECT_HANDOFF/139_FINALIZATION_WRITEOFF_FIRST_BATCH_FUSED_AUDIT_2026-08-26.md
docs/PROJECT_HANDOFF/138_REPAIR_COSTING_FAIL_CLOSED_REPAIR_LOOKUP_2026-08-26.md
docs/PROJECT_HANDOFF/137_MATERIAL_LEDGER_RETIRED_PRIVATE_HELPERS_REMOVAL_2026-08-26.md
docs/PROJECT_HANDOFF/136_MATERIAL_LEDGER_DEAD_ADJUSTMENT_HELPER_REMOVAL_2026-08-26.md
docs/PROJECT_HANDOFF/135_MATERIAL_LEDGER_FAIL_CLOSED_LOOKUPS_2026-08-26.md
docs/PROJECT_HANDOFF/134_WAREHOUSE_MOVEMENT_SUMMARY_SINGLE_PASS_2026-08-26.md
docs/PROJECT_HANDOFF/133_WAREHOUSE_PRICE_LOOKUP_VISIBILITY_2026-08-26.md
docs/PROJECT_HANDOFF/132_WAREHOUSE_FAIL_CLOSED_SPOOL_AND_DIAMETER_LOOKUPS_2026-08-26.md
docs/PROJECT_HANDOFF/131_WAREHOUSE_FAIL_CLOSED_REPAIR_LOOKUP_2026-08-26.md
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

Latest GREEN foundation = checkpoint **143**.

Latest verified evidence:

```text
final source through 143             6f69d0c548303cb9f6920b602cc0d8754deb5d5b
final contract                       38e892edc6a45d9540188516d06c6e41c93abd5d
ESP32 Build #1616                    33034665123 / SUCCESS
CMP Protocol Tests #3663             33034665166 / SUCCESS
CMP Protocol Tests #3664             33034707952 / SUCCESS
```

The earlier `#3657` startup failure and stuck `#3658/#1613` runs were GitHub Actions infrastructure failures before job execution; later successful runs above validate commits containing checkpoints 141-143.

## Current state

CRM client/motor existence lookups expose explicit `found` result channels. `JobSnapshotStore::exists()` is no longer public. Autonomous assignment exposes `assignMotorChecked()` publicly while the bool-only assignment path and completed-task lookup are internal.

Warehouse summary and first finalization write-off coverage batch reuse authoritative movement validation. Winding-session persistence keeps the read-only pre-begin directory preflight only for spool-selection, the only session store whose `begin()` may promote recoverable temp evidence.

Atomic RUN_WIRE remains the only current wire mutation path. Legacy public writeoff POST remains HTTP 410; historical GET/recovery compatibility remains intact.

## Immediate NEXT

1. Optimize `WindingJournal` repeated full scans of `/data/winding-runs/events.ndjson` by designing one authoritative streamed/bounded session-analysis pass.
2. Preserve duplicate/replay, exact run transition, session context, highest-run and completed-run integrity semantics.
3. Do not add unbounded vectors or whole-file buffering.
4. Preserve distinct Web HTTP preflight semantics and mutation-time TOCTOU validation.
5. Do not weaken integrity/provenance checks or add automatic rotation/deletion/truncation.
6. Continue software optimization before final two-board hardware E2E.

`RUN_COMPLETED` remains evidence only; no automatic writeoff, START or resume.
