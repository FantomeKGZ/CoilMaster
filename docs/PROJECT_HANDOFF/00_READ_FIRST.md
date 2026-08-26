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

Latest GREEN foundation = checkpoint **125**.

Latest verified checkpoint-125 evidence:

```text
price coordinator        74a92901262a060b203ea1b1a3cc3313537ce51a
price cross-log audit    84dabb9920cf60ca8cd8745b16a3e97e9093f50b
warehouse tag guard      4f20cc928723a0c3dd873741260ebed07d8690f5
final contracts          c799c74f3f8f4c6cbcd538ea662e3c86fe039304
ESP32 Build #1562        32960004843 / SUCCESS
ESP32 Build #1563        32960173338 / SUCCESS
ESP32 Build #1564        32960269882 / SUCCESS
CMP Protocol Tests #3514 32960004874 / SUCCESS
CMP Protocol Tests #3515 32960173324 / SUCCESS
CMP Protocol Tests #3516 32960270010 / SUCCESS
CMP Protocol Tests #3517 32960329745 / SUCCESS
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
```

Production operator route remains explicit and manual. Completed atomic evidence must agree on identity, quantity, timestamp, currency and price across Material Request, MaterialLedger and warehouse CONFIRMED records.

`RWI_TX=` is system-owned and is rejected by generic MaterialLedger usage and compatibility warehouse-writeoff Web input.

## Immediate NEXT

1. Audit every remaining caller of mutating `POST /api/warehouse/write-offs`.
2. If production has no remaining caller, disable the generic mutating Web route while preserving GET/history and internal warehouse recovery/store code.
3. Preserve fault/recovery tests that exercise store-level transaction semantics without requiring a public legacy mutation path.
4. Continue bounded read/report provenance work where useful.
5. Continue software work before final two-board hardware E2E.

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
