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

Stable pre-CRM snapshot:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
stable-2026-08-25-pre-crm-redesign -> same commit
```

## Read order

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/151_REPAIR_MATERIAL_SOFTWARE_ACCEPTANCE_2026-08-28.md
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

## Latest GREEN code checkpoint

Repair-material fail-closed operator UX is GREEN. Append-only generic corrections and RUN_WIRE operator provenance were re-audited and confirmed already authoritative.

```text
runtime/UI code SHA                    28343cb20cc5d75c748c38881d8705fd8505d182
ESP32 Build #1700                     run 33164496835 / SUCCESS
Arduino RU LCD Build #124             run 33164496837 / SUCCESS
final host-contract SHA               eff0cb8a7e0365cf6184aa81694d9e277037116d
CMP Protocol Tests #3867              run 33164535872 / SUCCESS
```

After those code/test SHAs only handoff/docs commits were added.

## Current state

The core software block from `07_REPAIR_MATERIAL_WRITEOFF_PLAN.md` is accepted complete for accounting/integrity. See `151_REPAIR_MATERIAL_SOFTWARE_ACCEPTANCE_2026-08-28.md`.

Generic repair-material usage keeps one explicit/manual mutation path with operation-id idempotency, authoritative stock/price preflight, persisted cost snapshots, final mutation-time TOCTOU/WAL and fail-closed recovery.

Append-only generic material corrections have exact source-usage provenance, bounded history, integrity/backup coverage and net RepairCosting. Confirmed usage is never edited/deleted. RUN_WIRE is not corrected through the generic path.

RUN_WIRE remains a separate exact-safe warehouse path with manual confirmation and exact `spool_id + source_session_id + source_run_id`. `RUN_COMPLETED` remains evidence only.

A convenience `material_not_found -> catalog/create` link is non-blocking UX polish only; automatic material creation is forbidden. A frequently-used-material shortcut is intentionally not implemented because no bounded/read-only aggregation API exists and a full growing-NDJSON scan/new favourites state is not justified.

Client PREPAYMENT remains separate from repair debt/costing and is never auto-applied.

## Immediate NEXT

1. Do not reopen the repair-material accounting/integrity block without a new concrete requirement.
2. Continue repo-reviewable software optimization/audit in `arduino-ru-lcd-experiment` using the current tree and existing handoff queue.
3. Prefer bounded single-pass/fused reads only when they do not combine different integrity domains or weaken fail-closed semantics.
4. Preserve append-only history, fixed RAM bounds and streamed growing-NDJSON reads.
5. No automatic rotation/deletion/truncation and no premature DB/index migration.
6. Hardware Arduino+ESP32 E2E for repair-material flow remains a separate final acceptance gate when hardware testing resumes.
7. Do not move experiment commits to `cmp-protocol-v1` without explicit approval.

`RUN_COMPLETED` remains evidence only; no automatic wire writeoff, physical START or resume.
