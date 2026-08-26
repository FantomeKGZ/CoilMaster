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

Latest GREEN foundation = checkpoint **135**.

Latest verified checkpoint-135 evidence:

```text
MaterialLedger API narrowing       6d1ca9a32611b0d0fc42ce4ed2aa1aa22e5d98d9
state/currency wrappers removed    4f92afd94aec4eca0c2fda4ec6ad7d13c9065e9c
fail-closed lookup contracts       1c9e2c6b402b28130ffd9e67d12a29b6918476e2
ESP32 Build #1587                  32971695182 / SUCCESS
CMP Protocol Tests #3601           32971743951 / SUCCESS
```

## Current state

Warehouse summary is single-pass through the authoritative movement codec/integrity path. MaterialLedger public repair/state/currency lookups now expose explicit `found` outputs, while dead material convenience wrappers are removed. The remaining one-argument MaterialLedger repair wrapper is private-only for the current internal `confirmUsage()` call.

Atomic RUN_WIRE remains the only current wire mutation path. Legacy public writeoff POST remains HTTP 410; historical GET/recovery compatibility remains intact.

## Immediate NEXT

1. Continue bounded runtime/API scan audit for safe duplicate-pass or ambiguous-read cleanup.
2. Do not remove the MaterialLedger Web currency preflight unless its distinct HTTP semantics and mutation TOCTOU protection can be preserved.
3. Do not weaken integrity/provenance checks or add automatic rotation/deletion/truncation.
4. Continue software optimization before final two-board hardware E2E.

`RUN_COMPLETED` remains evidence only; no automatic writeoff, START or resume.
