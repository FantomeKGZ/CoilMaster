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
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
```

Latest GREEN foundation = checkpoint **147**.

Latest verified evidence:

```text
final source through 147             a0ec58c552bed883d289fa1e30160f365955a6e1
final mandatory contract             6c404070d80532e617174d4621d828cf16a094c0
ESP32 Build #1624                    33036483178 / SUCCESS
CMP Protocol Tests #3695             source commit / SUCCESS
CMP Protocol Tests #3696             33036507740 / SUCCESS
```

CMP host audit remains **69 mandatory steps**.

## Current state

`WindingJournal` runtime and boot duplicate scans are consolidated. `CashPaymentStore::append()` fuses correction existence with next-id allocation. `MaterialRequestStatusStore::transition()` now resolves current status and next global transition id in one streamed pass of `material-request-status.ndjson`.

The immutable Material Request catalog lookup remains separate because it proves a different integrity domain. Public `resolve(..., found)` keeps missing-vs-read-failure semantics.

Atomic RUN_WIRE remains the only current wire mutation path. Legacy public writeoff POST remains HTTP 410; historical recovery compatibility remains intact.

## Immediate NEXT

1. Continue bounded audit of Material Request movement/coordinator and other append-only runtime stores for concrete same-file duplicate scans.
2. Do not combine scans across different ledgers merely to reduce I/O; preserve independent integrity domains.
3. Preserve Web HTTP preflight semantics and mutation-time TOCTOU validation.
4. Keep fixed RAM bounds; no whole-file buffering or unbounded vectors.
5. No automatic rotation/deletion/truncation or premature DB/index migration.
6. Continue software optimization before final two-board hardware E2E.

`RUN_COMPLETED` remains evidence only; no automatic writeoff, START or resume.
