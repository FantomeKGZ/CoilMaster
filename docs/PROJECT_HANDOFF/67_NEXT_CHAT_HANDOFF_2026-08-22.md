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

CRM/Web foundation is GREEN through checkpoint 116. The wire-accounting migration is GREEN through checkpoint 121:

```text
117 exact-spool / MaterialLedger owner map
118 append-only spool_id <-> warehouse_item_id bridge persistence + integrity + backup/export
119 backward-compatible MaterialLedger CU/AL + exact-diameter metadata
120 explicit operator-only runtime bridge creation
121 atomic explicit-operator RUN_WIRE ISSUE
```

Checkpoint 121 production route:

```text
POST /api/material-requests/warehouse
confirmed=true
movement_kind=ISSUE
source_kind=RUN_WIRE
unit=KG
```

Required exact provenance includes:

```text
material_request_id
repair_id
warehouse_item_id
source_session_id
source_run_id
spool_id
material_class=CU|AL
wire_diameter_hundredths_mm
quantity_milli_units=<actual consumed grams>
```

One authoritative `/data/workshop/run-wire-issue.pending.json` owns recovery across immutable Material Request movement, MaterialLedger usage and standard physical warehouse writeoff evidence. Exact physical spool before/after state is verified. Impossible ordering fails closed. Backup/restore is blocked while RUN_WIRE recovery intent exists.

Latest verified source/test evidence:

```text
source commit        db643d33cd5327556429e71f3734864c484d2f40
final test commit    7e73e9016c690e3ec65dfacfe3a80328b05a2148
ESP32 Build #1551    32951550134 / SUCCESS
CMP Tests #3475      32951582879 / SUCCESS
```

## Current production flow / migration boundary

```text
client -> motor -> OPEN repair -> linked winding
-> exact immutable spool selection -> UART JOB
-> physical START -> RUN_STARTED / RUN_COMPLETED
-> operator review
-> explicit atomic RUN_WIRE ISSUE OR legacy explicit exact-spool direct writeoff
-> standard physical CONFIRMED writeoff evidence
-> costing/finalization -> CLOSED -> reports -> backup
```

`RUN_COMPLETED` is still strictly non-mutating.

Checkpoint 121 retains standard physical KG_FIRST `CONFIRMED` evidence tied to exact `spool_id + source_session_id + source_run_id`, so current writeoff coverage/costing/finalization remains strict.

The legacy direct exact-spool writeoff path remains available until the operator/report migration is GREEN. Do not allow the old and new operator paths to double-account the same source run.

## Immediate NEXT

1. Audit current operator Web surfaces for legacy direct exact-spool writeoff versus atomic RUN_WIRE ISSUE.
2. Audit duplicate/double-accounting protection for the same `source_session_id + source_run_id` across both paths.
3. Make the atomic RUN_WIRE ISSUE usable from operator UI without automatic action on RUN_COMPLETED.
4. Audit reporting/costing/finalization so Material Request movement + Ledger usage + standard physical CONFIRMED evidence are not counted as three separate consumptions.
5. Preserve visible exact `material_request_id + source_session_id + source_run_id + spool_id + warehouse_item_id` provenance.
6. Keep legacy direct path until the transition is coherently GREEN.

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
- run-linked material evidence preserves exact source session/run/spool;
- restore is explicit/operator-only/transactional/fail-closed;
- backup/restore cannot cross unfinished RUN_WIRE transaction;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion or NDJSON truncation.

## Hardware release gate

Hardware E2E is separate from software migration. Full two-board verification remains mandatory once the CRM/material/writeoff contracts stabilize; do not infer hardware GREEN from CI.

## Read first in the next chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
docs/PROJECT_HANDOFF/121_RUN_WIRE_ISSUE_TRANSACTION_2026-08-26.md
docs/PROJECT_HANDOFF/120_OPERATOR_SPOOL_MATERIAL_BRIDGE_WEB_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```
