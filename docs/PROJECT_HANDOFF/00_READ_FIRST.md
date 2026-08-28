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
docs/PROJECT_HANDOFF/150_MATERIAL_FAIL_CLOSED_UX_2026-08-28.md
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

Repair-material fail-closed operator UX is complete. Existing append-only material corrections were also re-audited and confirmed already authoritative rather than a pending implementation task.

```text
runtime/UI code SHA                    28343cb20cc5d75c748c38881d8705fd8505d182
ESP32 Build #1700                     run 33164496835 / SUCCESS
Arduino RU LCD Build #124             run 33164496837 / SUCCESS
final host-contract SHA               eff0cb8a7e0365cf6184aa81694d9e277037116d
CMP Protocol Tests #3867              run 33164535872 / SUCCESS
```

See `150_MATERIAL_FAIL_CLOSED_UX_2026-08-28.md` for exact error/operation-id semantics and the corrected next software audit.

The preceding client editing + PREPAYMENT block remains GREEN and is documented in `149_CLIENT_EDIT_PREPAYMENT_2026-08-28.md`.

## Current state

Generic repair-material usage keeps one manual mutation path. `material_state_unavailable_after_replay` is treated as a potentially already-durable operation: the operator is explicitly told not to create a new `operation_id`. Authoritative/idempotency read failures remain fail-closed and preserve the same operation identity; proven pre-mutation no-write cases require a fresh manual selection/confirmation. No automatic retry was added.

Append-only generic material corrections are already implemented with exact source-usage provenance, idempotent replay, bounded history, integrity/backup coverage and net RepairCosting. Confirmed source usage is never edited or deleted. Generic corrections remain isolated from RUN_WIRE evidence and warehouse correction semantics.

Client identity remains stable and edits are append-only revisions. PREPAYMENT remains separate from repair debt/costing and is never auto-applied.

Atomic RUN_WIRE remains the only current wire mutation path. Legacy public writeoff POST remains disabled; exact spool/session/run provenance and historical recovery compatibility remain intact.

## Immediate NEXT

1. Audit operator visibility in the unified repair-material card rather than reimplementing the already-existing correction ledger.
2. Verify that each generic usage can be clearly related to its append-only corrections and net quantity/cost in the UI.
3. Verify that RUN_WIRE provenance/history is visible enough from the same repair context while remaining a separate warehouse ledger/path.
4. Check actionable navigation for `material_not_found` / missing stock position using only existing safe warehouse/create workflows.
5. Consider a bounded/read-only frequently-used material shortcut only if it requires no new duplicated state or unbounded scan.
6. Preserve append-only history, bounded streamed reads and fail-closed integrity semantics.
7. Keep fixed RAM bounds; no whole-file buffering or unbounded vectors.
8. No automatic rotation/deletion/truncation or premature DB/index migration.
9. Continue software work in `arduino-ru-lcd-experiment`; do not move changes to production without explicit approval.

`RUN_COMPLETED` remains evidence only; no automatic wire writeoff, physical START or resume.
