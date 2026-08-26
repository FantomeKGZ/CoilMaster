# CoilMaster — completion estimate and next-chat transfer

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**  
Repo: `FantomeKGZ/CoilMaster`

## Stable baseline / source rule

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable pre-CRM
stable-2026-08-25-pre-crm-redesign -> same commit
```

Вся разработка после snapshot идёт только в `cmp-protocol-v1`. `main` не использовать как source.

## Current software state

CRM/Web is GREEN through checkpoint 116. Wire-accounting migration is GREEN through checkpoint 121:

```text
117 forensic exact-spool / MaterialLedger owner map
118 spool_id <-> warehouse_item_id persistence bridge + bounded integrity + backup/export
119 backward-compatible MaterialLedger structured wire metadata
120 explicit operator-only spool-material bridge creation
121 atomic explicit-operator RUN_WIRE ISSUE across Material Request + MaterialLedger + exact physical spool
```

Latest verified source/test evidence:

```text
source commit        db643d33cd5327556429e71f3734864c484d2f40
final test commit    7e73e9016c690e3ec65dfacfe3a80328b05a2148
ESP32 Build #1551    32951550134 / SUCCESS
CMP Tests #3475      32951582879 / SUCCESS
```

Checkpoint 121 adds one authoritative durable RUN_WIRE pending and exact recovery across immutable Material Request movement, MaterialLedger usage, standard warehouse confirmed writeoff evidence and exact physical spool before/after state. Backup/restore is blocked while recovery intent exists.

`RUN_COMPLETED` is still strictly non-mutating. Stock changes only from explicit operator-confirmed RUN_WIRE ISSUE.

## Current mandatory work

Complete the operator/report side of the new atomic RUN_WIRE path.

Target contract:

```text
RUN_COMPLETED -> non-mutating
operator review
explicit RUN_WIRE ISSUE
material_request_id
source_session_id + source_run_id
exact spool_id
warehouse_item_id via bridge
CU/AL + exact diameter
actual consumed grams
one standard physical confirmed writeoff
one MaterialLedger usage
one Material Request movement
```

Audit old direct exact-spool writeoff versus new RUN_WIRE for duplicate/double-accounting risk. Keep the legacy direct path until UI/report/costing/finalization behavior is coherently GREEN.

## Safety invariants

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never auto-deducts material;
- warehouse ISSUE requires explicit operator action;
- exact spool/session/run provenance remains mandatory;
- cancellation/operator abort preserves immutable history;
- restore operator-only, transactional, fail-closed;
- backup/restore blocks on unfinished RUN_WIRE recovery;
- no automatic production deletion/truncation.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
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
this file
```

## Continuation prompt

```text
Продолжаем CoilMaster. Source-of-truth только cmp-protocol-v1; main не использовать. Checkpoint 121 GREEN: source db643d33cd5327556429e71f3734864c484d2f40, final test 7e73e9016c690e3ec65dfacfe3a80328b05a2148, ESP32 #1551 / 32951550134 SUCCESS, CMP #3475 / 32951582879 SUCCESS. Atomic explicit RUN_WIRE ISSUE now requires exact material_request_id + source_session_id + source_run_id + spool_id + bridged warehouse_item_id + CU/AL + diameter + actual grams. One durable RUN_WIRE pending owns recovery across Material Request movement, Ledger usage and standard physical warehouse writeoff; backup blocks during recovery. RUN_COMPLETED remains non-mutating. Next: operator UI/report/costing/finalization duplicate-accounting audit, while keeping legacy direct exact-spool writeoff until migration is GREEN. Physical START local-only; Arduino owns SSR.
```
