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

Latest GREEN foundation = checkpoint **134**.

Latest verified checkpoint-134 evidence:

```text
final source                       e734ad77ee232a9ed17b1118179e1ffd96666cc5
ESP32 Build #1585                  32967638219 / SUCCESS
CMP Protocol Tests #3594           32967638259 / SUCCESS
```

## Current state

Warehouse summary now reuses the authoritative `WarehouseMovementIntegrityAudit` codec/integrity pass to obtain per-diameter month/all-time consumption. The former second manual full parse of `movements.ndjson` is removed. Exact provenance uniqueness remains fail-closed and bounded. The one-argument warehouse price wrapper is also fully removed.

Atomic RUN_WIRE remains the only current wire mutation path. Legacy public writeoff POST remains HTTP 410; historical GET/recovery compatibility remains intact.

## Immediate NEXT

1. Audit MaterialLedger/runtime for duplicate validated full-log passes before reads/mutations.
2. Reuse existing one-pass preflight evidence where possible; do not weaken integrity/provenance checks.
3. Do not add automatic rotation/deletion/truncation or premature DB migration.
4. Continue software optimization before final two-board hardware E2E.

`RUN_COMPLETED` remains evidence only; no automatic writeoff, START or resume.
