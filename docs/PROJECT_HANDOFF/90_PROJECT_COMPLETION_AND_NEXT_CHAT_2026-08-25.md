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
```

Latest verified:

```text
CMP Protocol Tests 32939884633 / SUCCESS
ESP32 Build         32939884635 / SUCCESS
```

Checkpoint 118 keeps current runtime safety intact: no bridge writer API, no automatic material mutation, no relaxation of exact-spool writeoff/finalization.

## Current mandatory work

Extend authoritative `MaterialLedger` backward-compatibly with structured wire metadata:

```text
wire_type = CU | AL
diameter_hundredths_mm = exact diameter
```

Requirements:
- existing generic/non-wire materials remain valid unchanged;
- existing integer quantity/cost contracts remain unchanged;
- bridge creation must later require exact agreement between physical spool metadata and MaterialLedger wire metadata;
- only explicit operator action may create the bridge;
- old exact-spool runtime remains authoritative until the coordinated migration is finished.

After that, migrate run-linked wire accounting toward explicit Material Request `RUN_WIRE` ISSUE with exact `material_request_id + source_session_id + source_run_id + physical spool provenance + actual weight`.

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
docs/PROJECT_HANDOFF/118_SPOOL_MATERIAL_BRIDGE_PERSISTENCE_2026-08-26.md
docs/PROJECT_HANDOFF/117_SPOOL_TO_MATERIAL_REQUEST_MIGRATION_MAP_2026-08-26.md
docs/PROJECT_HANDOFF/116_CASH_WEB_UI_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
this file
```

## Continuation prompt

```text
Продолжаем CoilMaster. Source-of-truth только cmp-protocol-v1; main не использовать. Checkpoint 118 GREEN: CMP 32939884633 SUCCESS, ESP32 32939884635 SUCCESS. Есть append-only spool_id <-> warehouse_item_id bridge с bounded integrity и backup/export, но runtime writer ещё не открыт и exact-spool writeoff/finalization не ослаблены. Следующий блок — backward-compatible structured CU/AL + diameter metadata в authoritative MaterialLedger, затем fail-closed operator bridge creation, затем coordinated Material Request RUN_WIRE migration. RUN_COMPLETED ничего автоматически не списывает; physical START local-only; Arduino owns SSR.
```
