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
docs/PROJECT_HANDOFF/152_RUN_WIRE_MATERIAL_REQUEST_STATUS_BATCH_2026-08-28.md
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

RUN_WIRE Material Request status preflight no longer performs N+1 server-side status scans. A bounded max-24 batch resolver performs one request-journal scan plus one status-journal scan per page while preserving global ordering and exact per-request lifecycle-chain validation.

```text
runtime/UI code SHA                    104f319e1cb8e466a229c0d40876b35aac6ded86
ESP32 Build #1708                     run 33167009269 / SUCCESS
Arduino RU LCD Build #132             run 33167009264 / SUCCESS
final host-contract SHA               17c7c43058802f44d5f0b26d0da301190e792ebd
CMP Protocol Tests #3885              run 33167039155 / SUCCESS
```

See `152_RUN_WIRE_MATERIAL_REQUEST_STATUS_BATCH_2026-08-28.md`.

## Current state

The repair-material software accounting/integrity block is accepted complete; see checkpoint 151. Generic material usage/corrections remain append-only, idempotent and fail-closed. RUN_WIRE remains a separate manual exact-safe warehouse path.

RUN_WIRE status batching is read-only optimization only. Exact `source_session_id + source_run_id + spool_id`, explicit `confirmed=true`, dedicated `/api/material-requests/warehouse`, and `RUN_COMPLETED` evidence-only semantics are unchanged.

Client edits remain append-only revisions. Client PREPAYMENT remains separate from repair debt/costing and is never auto-applied.

## Immediate NEXT

1. Continue repo-reviewable repeated-scan/performance audit in `arduino-ru-lcd-experiment`.
2. First candidate: exact RUN_WIRE spool lookup — verify whether Web still walks the whole paged spool catalogue for one immutable `spool_id` and whether an existing authoritative by-id backend lookup can replace it safely.
3. Prefer bounded/single-pass reads only when integrity domains remain separate and fail-closed semantics are preserved.
4. Keep fixed RAM bounds; no whole-file buffering, unbounded vectors, automatic rotation/deletion/truncation or premature DB/index migration.
5. Hardware Arduino+ESP32 E2E remains a separate final gate.
6. Do not move experiment commits to `cmp-protocol-v1` without explicit approval.

`RUN_COMPLETED` remains evidence only; no automatic wire writeoff, physical START or resume.
