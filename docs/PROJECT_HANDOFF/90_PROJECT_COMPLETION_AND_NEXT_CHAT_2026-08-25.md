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

CRM/Web is GREEN through checkpoint 116. Wire-accounting migration has advanced through:

```text
117 forensic exact-spool / MaterialLedger owner map
118 spool_id <-> warehouse_item_id persistence bridge + bounded integrity + backup/export
119 backward-compatible MaterialLedger structured wire metadata
120 explicit operator-only spool-material bridge creation in production Web bootstrap
```

Latest verified on final checkpoint-120 tree `fa651e3e50a25df9489db24b6c71bd853171a9b8`:

```text
CMP Protocol Tests 32944119683 / SUCCESS
ESP32 Build         32944119688 / SUCCESS
```

Checkpoint 120 preserves the current safety boundary: bridge creation requires explicit `confirm=1`, exact ACTIVE spool/material agreement and appends identity evidence only. It does not mutate stock and does not relax exact-spool writeoff/finalization.

## Current mandatory work

Build the coordinated crash-safe run-linked Material Request `RUN_WIRE` migration.

First inspect the existing MaterialLedger usage/adjustment transaction semantics and current Material Request warehouse coordinator. Do not create a split transaction where one stock/evidence domain commits while the other can be lost on reboot.

Target contract:

```text
RUN_COMPLETED -> non-mutating
explicit operator ISSUE
material_request_id
source_session_id + source_run_id
exact physical spool_id provenance through bridge
CU/AL + exact diameter
actual consumed weight
manual confirmation
pending/recovery evidence
```

Current exact-spool runtime remains authoritative until the coordinated migration is complete across movement/costing/finalization/backup/integrity/reports/Web/tests.

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
- cancellation/operator abort preserves immutable history;
- restore operator-only, transactional, fail-closed;
- no automatic production deletion/truncation.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
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
Продолжаем CoilMaster. Source-of-truth только cmp-protocol-v1; main не использовать. Checkpoint 120 GREEN: CMP 32944119683 SUCCESS, ESP32 32944119688 SUCCESS, final verified tree fa651e3e50a25df9489db24b6c71bd853171a9b8. Есть append-only spool_id <-> warehouse_item_id bridge, authoritative MaterialLedger CU/AL+diameter metadata и explicit operator-only POST /api/warehouse/spool-material-bridges; bridge creation stock не меняет. Следующий блок — crash-safe coordinated Material Request RUN_WIRE ISSUE с exact material_request_id + source_session_id + source_run_id + physical spool provenance. Старый exact-spool writeoff/finalization не ослаблять до полной end-to-end миграции. RUN_COMPLETED ничего автоматически не списывает; physical START local-only; Arduino owns SSR.
```
