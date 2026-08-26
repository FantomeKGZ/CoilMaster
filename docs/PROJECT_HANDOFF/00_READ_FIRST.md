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

Latest GREEN foundation = checkpoint **124**.

Latest verified checkpoint-124 evidence:

```text
cross-log source        9448c250955664c7e82a5e69ba26a569d3b93fe7
workshop integration    eef4157ac13e09d5636faa49817baa5a63cfc794
contract coverage       63ac31dc37f2542e3879466df9158312ac21a2f6
ESP32 Build #1560       32959482667 / SUCCESS
ESP32 Build #1561       32959521066 / SUCCESS
CMP Protocol Tests #3507 32959482741 / SUCCESS
CMP Protocol Tests #3509 32959605104 / SUCCESS
```

## Current migration state

```text
118 spool <-> MaterialLedger bridge persistence
119 authoritative wire metadata
120 explicit operator bridge creation
121 atomic RUN_WIRE transaction/recovery
122 desktop/mobile operator writeoff migrated to atomic RUN_WIRE
123 costing/finalization accounting converged: RUN_WIRE counted once
124 bounded cross-log RUN_WIRE integrity across request + ledger + physical writeoff
```

Production operator route remains explicit and manual:

```text
RUN_COMPLETED (read-only evidence)
-> exact immutable spool + exact bridge
-> explicit DRAFT/ISSUED Material Request
-> explicit atomic RUN_WIRE ISSUE
-> one Material Request movement
-> one MaterialLedger usage tagged RWI_TX
-> one confirmed physical warehouse writeoff
```

Checkpoint 124 additionally requires completed evidence to agree on exact transaction, repair, warehouse item, session/run, physical spool, CU/AL, diameter, consumed quantity, currency and timestamp. In-flight pending/tmp is never guessed by the audit; coordinator recovery remains authoritative.

## Immediate NEXT

1. Enforce one price authority for atomic RUN_WIRE: MaterialLedger KG-equivalent price must equal the warehouse KG price used by physical writeoff/costing.
2. Reserve `RWI_TX=` on compatibility direct writeoff comments so generic callers cannot spoof system accounting provenance.
3. Expose bounded read/report provenance without introducing a second accounting ledger.
4. Review safe formal deprecation boundary for legacy mutating writeoff POST while preserving historical GET/read compatibility.
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
