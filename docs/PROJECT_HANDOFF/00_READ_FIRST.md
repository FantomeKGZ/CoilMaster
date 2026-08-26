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
docs/PROJECT_HANDOFF/121_RUN_WIRE_ISSUE_TRANSACTION_2026-08-26.md
docs/PROJECT_HANDOFF/120_OPERATOR_SPOOL_MATERIAL_BRIDGE_WEB_2026-08-26.md
docs/PROJECT_HANDOFF/119_MATERIAL_LEDGER_WIRE_METADATA_2026-08-26.md
docs/PROJECT_HANDOFF/118_SPOOL_MATERIAL_BRIDGE_PERSISTENCE_2026-08-26.md
docs/PROJECT_HANDOFF/117_SPOOL_TO_MATERIAL_REQUEST_MIGRATION_MAP_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

Latest GREEN foundation = checkpoint **121**.

Latest verified source/test evidence:

```text
source commit        db643d33cd5327556429e71f3734864c484d2f40
final test commit    7e73e9016c690e3ec65dfacfe3a80328b05a2148
ESP32 Build #1551    32951550134 / SUCCESS
CMP Tests #3475      32951582879 / SUCCESS
```

## Current migration state

The exact physical spool ↔ MaterialLedger migration now has a crash-safe operator mutation path:

```text
118 persistence + bounded integrity + backup/export
119 authoritative backward-compatible MaterialLedger wire metadata
120 explicit operator-only spool-material bridge creation
121 explicit operator-only atomic RUN_WIRE ISSUE
```

RUN_WIRE production route:

```text
POST /api/material-requests/warehouse
confirmed=true
movement_kind=ISSUE
source_kind=RUN_WIRE
unit=KG
spool_id=<exact physical spool>
source_session_id=<exact session>
source_run_id=<exact run>
```

Checkpoint 121 validates exact immutable spool selection, exact completed run, bridge and ACTIVE MaterialLedger `GRAM + CU/AL + diameter`. One durable RUN_WIRE pending owns recovery across Material Request movement, MaterialLedger usage and standard physical warehouse writeoff evidence.

Existing exact-spool finalization remains strict. The standard physical `CONFIRMED` writeoff evidence is still emitted, so downstream writeoff coverage/costing/finalization does not lose exact run provenance.

## Immediate NEXT

1. Audit Material Request / wire reporting and operator UI for the new atomic RUN_WIRE ISSUE.
2. Surface explicit exact-spool RUN_WIRE ISSUE without creating any automatic `RUN_COMPLETED` mutation.
3. Keep exact `material_request_id + source_session_id + source_run_id + spool_id` visible in operator/reports evidence.
4. Preserve bridge CU/AL + diameter and actual consumed weight.
5. Keep the legacy direct exact-spool writeoff path until the operator/report migration is coherently GREEN.
6. Continue software work before the final two-board hardware E2E.

## Material safety

`RUN_COMPLETED` never automatically deducts material. Warehouse mutation requires explicit operator action. RUN_WIRE now requires exact physical spool/run provenance and a crash-safe high-level pending owner.

## General safety invariants

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- backup/restore is blocked while RUN_WIRE recovery intent exists;
- no automatic production deletion/truncation.

## Working discipline

Before modifying an existing file: fetch exact `cmp-protocol-v1` content + current blob SHA. Before a new path: confirm 404. Never claim GREEN without actual CI/build evidence.
