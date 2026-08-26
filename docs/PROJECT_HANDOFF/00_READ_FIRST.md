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
docs/PROJECT_HANDOFF/127_RUN_WIRE_PERSISTED_SPOOL_INTEGRITY_2026-08-26.md
docs/PROJECT_HANDOFF/126_RUN_WIRE_READ_PROVENANCE_AND_LEGACY_POST_DEPRECATION_2026-08-26.md
docs/PROJECT_HANDOFF/125_RUN_WIRE_PRICE_PROVENANCE_CONVERGENCE_2026-08-26.md
docs/PROJECT_HANDOFF/124_RUN_WIRE_CROSS_LOG_INTEGRITY_2026-08-26.md
docs/PROJECT_HANDOFF/123_RUN_WIRE_ACCOUNTING_CONVERGENCE_2026-08-26.md
docs/PROJECT_HANDOFF/122_RUN_WIRE_OPERATOR_UI_MIGRATION_2026-08-26.md
docs/PROJECT_HANDOFF/121_RUN_WIRE_ISSUE_TRANSACTION_2026-08-26.md
docs/PROJECT_HANDOFF/120_OPERATOR_SPOOL_MATERIAL_BRIDGE_WEB_2026-08-26.md
docs/PROJECT_HANDOFF/119_MATERIAL_LEDGER_WIRE_METADATA_2026-08-26.md
docs/PROJECT_HANDOFF/118_SPOOL_MATERIAL_BRIDGE_PERSISTENCE_2026-08-26.md
docs/PROJECT_HANDOFF/117_SPOOL_TO_MATERIAL_REQUEST_MIGRATION_MAP_2026-08-26.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

Latest GREEN foundation = checkpoint **127**.

Latest verified checkpoint-127 evidence:

```text
persisted spool audit     6965bd716ac9f4d3970bc750a8e8933b7b6fffd0
mandatory contract        38883ae01493622d1bc98fc179fb9d9eb571ddcf
ESP32 Build #1570         32961925117 / SUCCESS
CMP Protocol Tests #3541  32961999553 / SUCCESS
```

## Current migration state

```text
118 spool <-> MaterialLedger bridge persistence
119 authoritative wire metadata
120 explicit operator bridge creation
121 atomic RUN_WIRE transaction/recovery
122 desktop/mobile operator writeoff migrated to atomic RUN_WIRE
123 costing/finalization accounting converged: RUN_WIRE counted once
124 bounded cross-log RUN_WIRE integrity
125 one KG wire price + reserved system accounting provenance
126 direct exact spool provenance in new RUN_WIRE movements + legacy POST 410 deprecation
127 optional persisted spool_id is cross-checked against immutable selection in the existing bounded audit pass
```

New RUN_WIRE movement `spool_id` cannot disagree with immutable `JobSpoolSelection`. Historical movements without the field remain valid and use the immutable selection as authority. No additional full movement-log scan was added.

Public `POST /api/warehouse/write-offs` remains permanently fail-closed with HTTP 410. GET/history and deterministic low-level recovery remain intact.

## Immediate NEXT

1. Review bounded read/report surfaces for places where direct transaction provenance avoids ambiguous reconstruction.
2. Reuse existing cross-log/read batches; do not add redundant full-log scans.
3. Review whether retained low-level legacy writeoff APIs can be narrowed to managed/recovery-only use without breaking history/recovery.
4. Continue software/integrity optimization before final two-board hardware E2E.

## Material safety

`RUN_COMPLETED` never automatically deducts material. Warehouse mutation requires explicit operator action and exact Material Request / spool / session / run provenance.

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
