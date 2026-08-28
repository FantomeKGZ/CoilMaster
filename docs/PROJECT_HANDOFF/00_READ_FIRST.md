# CoilMaster — current project entrypoint

Дата обновления: **2026-08-28**  
Repo: `FantomeKGZ/CoilMaster`  
Production/source-of-truth: **`cmp-protocol-v1`**. `main` для исходников не использовать.  
Текущая рабочая ветка: **`arduino-ru-lcd-experiment`**.

## Branch policy

Production остаётся на:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

Все новые изменения текущего цикла выполняются в `arduino-ru-lcd-experiment`. Не переносить их обратно в `cmp-protocol-v1` без отдельного прямого запроса пользователя.

Stable pre-CRM snapshot сохранён как историческая контрольная точка:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
stable-2026-08-25-pre-crm-redesign -> same commit
```

## Read order

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/149_CLIENT_EDIT_PREPAYMENT_2026-08-28.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md
docs/PROJECT_HANDOFF/147_MATERIAL_REQUEST_STATUS_TRANSITION_SINGLE_PASS_2026-08-27.md
docs/PROJECT_HANDOFF/146_CASH_PAYMENT_APPEND_SINGLE_PASS_2026-08-27.md
docs/PROJECT_HANDOFF/145_WINDING_JOURNAL_BOOT_SINGLE_PASS_2026-08-27.md
docs/PROJECT_HANDOFF/144_WINDING_JOURNAL_RUNTIME_SINGLE_PASS_2026-08-27.md
docs/PROJECT_HANDOFF/143_AUTONOMOUS_ASSIGNMENT_API_NARROWING_2026-08-27.md
docs/PROJECT_HANDOFF/142_JOB_SNAPSHOT_EXISTS_VISIBILITY_2026-08-27.md
docs/PROJECT_HANDOFF/141_CRM_FAIL_CLOSED_SOURCE_LOOKUPS_2026-08-26.md
docs/PROJECT_HANDOFF/140_WINDING_SESSION_SELECTION_ONLY_PREFLIGHT_2026-08-26.md
docs/PROJECT_HANDOFF/139_FINALIZATION_WRITEOFF_FIRST_BATCH_FUSED_AUDIT_2026-08-26.md
docs/PROJECT_HANDOFF/138_REPAIR_COSTING_FAIL_CLOSED_REPAIR_LOOKUP_2026-08-26.md
docs/PROJECT_HANDOFF/137_MATERIAL_LEDGER_RETIRED_PRIVATE_HELPERS_REMOVAL_2026-08-26.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
```

## Latest GREEN experiment checkpoint

Client editing + explicit client PREPAYMENT are complete in the experiment branch.

```text
runtime/UI code SHA                    d47b2f5a088566af33f20e805104206b9c167fe2
ESP32 Build #1699                     run 33162070105 / SUCCESS
Arduino RU LCD Build #123             run 33162070106 / SUCCESS
final host-contract SHA               58b1aff87434dbb38a16327c7929b2a61b4befc9
CMP Protocol Tests #3863              run 33162276329 / SUCCESS
```

See `149_CLIENT_EDIT_PREPAYMENT_2026-08-28.md` for exact storage/API/UI semantics and commit lineage.

## Current state

Client identity remains stable and edits are append-only revisions. `client-details.html` is now the deliberate write surface for client edits; the client catalogue itself remains read-only.

Client PREPAYMENT reuses the existing authoritative cash journal. It is `ADD` only, has exact `client_id`, uses `repair_id=0`, requires explicit confirmation, and is kept in a separate `prepayment_minor` bucket. It is not automatically applied to repair debt and does not affect RepairCosting.

Repair-material work remains based on existing MaterialLedger/Warehouse/RepairCosting stores. No parallel stock ledger was introduced.

Atomic RUN_WIRE remains the only current wire mutation path. Legacy public writeoff POST remains disabled; exact spool/session/run provenance and historical recovery compatibility remain intact.

## Immediate NEXT

1. Return to the active repair-material plan in `07_REPAIR_MATERIAL_WRITEOFF_PLAN.md` from the latest recorded checkpoint.
2. Keep client PREPAYMENT separate until/unless an explicit operator-driven allocation-to-repair workflow is requested; never auto-apply it.
3. Preserve append-only history, bounded streamed reads and fail-closed integrity semantics.
4. Do not combine scans across different ledgers merely to reduce I/O.
5. Preserve Web HTTP preflight semantics and mutation-time TOCTOU validation.
6. Keep fixed RAM bounds; no whole-file buffering or unbounded vectors.
7. No automatic rotation/deletion/truncation or premature DB/index migration.
8. Continue software work in `arduino-ru-lcd-experiment`; do not move changes to production without explicit approval.

`RUN_COMPLETED` remains evidence only; no automatic wire writeoff, physical START or resume.
