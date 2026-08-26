# Next chat handoff — CoilMaster current transfer — 2026-08-26

## Source of truth

```text
Repository: FantomeKGZ/CoilMaster
Branch: cmp-protocol-v1
main: DO NOT use as source code
stable pre-CRM: 449570d47649d5f6336a31ee3eed491256e0fb1a
```

Before every edit/delete fetch the current file from `cmp-protocol-v1` and use its current blob SHA. Verify a new path is absent before creating it.

## Historical cleanup state

The full repo-level audit and controlled cleanup completed on 2026-08-23. Do not restart broad cleanup without a concrete defect or stale-contract finding.

Stable cleanup evidence remains historical. Current product development after the pre-CRM snapshot lives only on `cmp-protocol-v1`.

## Current GREEN product state

CRM/Web foundation is GREEN through checkpoint 116. The wire-accounting migration is GREEN through checkpoint 120:

```text
117 exact-spool / MaterialLedger owner map
118 append-only spool_id <-> warehouse_item_id bridge persistence + integrity + backup/export
119 backward-compatible MaterialLedger CU/AL + exact-diameter metadata
120 explicit operator-only runtime bridge creation
```

Checkpoint 120 production route:

```text
POST /api/warehouse/spool-material-bridges
```

The route requires explicit `confirm=1`, an ACTIVE physical spool, an ACTIVE MaterialLedger `GRAM` wire item, and exact CU/AL + diameter agreement. It appends bridge identity evidence only; stock is not mutated.

Final checkpoint-120 verified tree:

```text
fa651e3e50a25df9489db24b6c71bd853171a9b8
CMP Protocol Tests 32944119683 / SUCCESS
ESP32 Build         32944119688 / SUCCESS
```

## Current production flow / migration boundary

Current production writeoff/finalization still uses the exact immutable physical spool path:

```text
client -> motor -> OPEN repair -> linked winding
-> exact immutable spool selection -> UART JOB
-> physical START -> RUN_STARTED / RUN_COMPLETED
-> explicit manual exact-run exact-spool writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

Checkpoint 120 does **not** replace this path yet.

Target migration:

```text
RUN_COMPLETED -> non-mutating
explicit operator RUN_WIRE ISSUE
material_request_id
source_session_id + source_run_id
exact physical spool provenance through spool-material bridge
CU/AL + exact diameter
actual consumed weight
manual confirmation
crash-safe pending/recovery
```

Do not partially remove existing exact-spool writeoff/finalization checks before the coordinated migration is complete across warehouse/material movement, Material Request, costing, finalization, backup/integrity, reports, Web and tests.

## Immediate NEXT

1. Inspect existing `MaterialLedger` usage/adjustment transaction semantics.
2. Inspect the current Material Request warehouse coordinator and pending/recovery store.
3. Select one authoritative crash-safe transaction boundary for run-linked wire ISSUE.
4. Avoid any sequence where physical stock and immutable Material Request evidence can diverge after a crash/reboot.
5. Preserve exact `material_request_id + source_session_id + source_run_id + spool_id` provenance through the bridge.

## Non-negotiable safety rules

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly drive SSR;
- timeout/lost ACK alone never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never automatically deducts material;
- warehouse mutation requires explicit operator action;
- run-linked material evidence preserves exact source session/run;
- current linked writeoff retains exact immutable spool provenance until migration completes;
- restore is explicit/operator-only/transactional/fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion or NDJSON truncation.

## Hardware release gate

Hardware E2E is separate from software migration. Full two-board verification remains mandatory once the CRM/material/writeoff contracts stabilize; do not infer hardware GREEN from CI.

## Read first in the next chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/120_OPERATOR_SPOOL_MATERIAL_BRIDGE_WEB_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```
