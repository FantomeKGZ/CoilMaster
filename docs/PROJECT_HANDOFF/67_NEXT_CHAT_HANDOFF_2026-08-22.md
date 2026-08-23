# Next chat handoff — CoilMaster cleanup complete — 2026-08-23

## Source of truth

```text
Repository: FantomeKGZ/CoilMaster
Branch: cmp-protocol-v1
main: DO NOT use as source code
```

Before every edit/delete fetch the current file from `cmp-protocol-v1` and use its current blob SHA. Verify a path is absent before creating a new file.

## Software cleanup checkpoint

**SOFTWARE CLEANUP COMPLETE — 100%.**

The latest cleanup implementation/test batch was confirmed GREEN by the operator on **2026-08-23**: the user explicitly reported that all current GitHub Actions are green.

Latest implementation/test cleanup commit:

```text
bd64e3cc4ba92a6624aed677d98c1620c165013e
test(warehouse): guard against duplicate web bootstrap
```

Protected implementation fix:

```text
06a752663504d58ca6908414f8aa8786007c6877
fix(esp32): remove duplicate warehouse web bootstrap
```

Later commits in the checkpoint sequence are documentation/status synchronization only.

## Final classification

```text
DELETE  no remaining proven cleanup candidates
MERGE   no duplicate authoritative owners remain
KEEP    reviewed live production/build/test/docs/recovery owners
REVIEW  none remain in the named cleanup queue
FIXED   warehouse duplicate Web bootstrap + regression contract
CI      current Actions confirmed GREEN by operator on 2026-08-23
```

Do not restart broad repository cleanup without a concrete new inconsistency, failing test, runtime defect or stale-contract evidence.

## Current production flow

```text
client
-> motor
-> OPEN repair
-> costing
-> linked winding
-> exact immutable spool selection
-> immutable snapshot/state
-> UART JOB
-> physical START
-> RUN_STARTED / RUN_COMPLETED
-> explicit manual exact-run exact-spool wire writeoff
-> costing / finalization preflight
-> CLOSED
-> reports
-> backup
```

## Non-negotiable safety rules

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly drive SSR;
- timeout/lost ACK alone never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never automatically deducts wire;
- current linked writeoff is exact `source_session_id + source_run_id + immutable spool_id`;
- historical `UNALLOCATED` is read/audit/recovery compatibility only;
- restore is explicit/operator-only/transactional/fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion or NDJSON truncation.

## Final owner sweep

The final build-included owner sweep closed the remaining named groups as intentional `KEEP`, including:

- `RepairRegistry` core/lookup/page/search/similarity;
- `WindingJournalQuery` + `WindingJournalQueryValidation`;
- `WindingJournalTransitionAudit`;
- `WindingJournalSnapshotContext`;
- `RepairCosting` + `RepairCostingValidation`;
- `AutonomousWindingArchive` core/assign/integrity/page;
- active `CM_WarehouseLegacySpoolMaterial.cpp` migration owner;
- `CM_JobSpoolSelectionLookup.cpp`;
- `CM_WarehouseWriteOffLookup.cpp`;
- current build-included `Core/` and `Arduino/` owner trees.

No deletion was based only on a filename or empty GitHub search result.

## Warehouse duplicate bootstrap defect — closed

`main.cpp` intentionally calls both:

```text
warehouseWeb.begin();
warehouseWeb.beginSpoolList();
```

Current ownership after `06a7526...`:

```text
WarehouseWeb::begin()
  common warehouse/write-off/conductor/settings/material services

WarehouseWeb::beginSpoolList()
  GET  /api/warehouse/spools
  POST /api/warehouse/spools/material
  GET  /api/warehouse/material-summary
```

Regression `bd64e3c...` is in the already executed `Tests/Web/check_warehouse_spool_list_cleanup.js` and prevents duplicate bootstrap from returning.

## Crash-residue classification — final

```text
JobStateStore .tmp/.bak        KEEP fail-closed
JobSpoolSelection .json.tmp    KEEP bounded recovery before UART
JobSnapshot .json.tmp          KEEP non-authoritative preparation evidence; no auto-promote/resume/delete
```

These policies remain intentionally different because their durable transaction boundaries differ.

## Hardware release gate

Hardware E2E is **separate from software cleanup**. For final physical release confidence, targeted two-board ESP32<->Arduino smoke/recovery verification remains the external gate when needed:

- UART job delivery;
- physical START remains local/physical;
- repeat behavior;
- cancel behavior;
- reboot/no-auto-resume behavior;
- exact run/session evidence continuity.

Do not infer hardware GREEN from CI. Do not request broad logs unless a concrete hardware-only problem appears.

## Next work

Software cleanup is closed. Continue only from a concrete product/runtime goal, hardware verification result, bug, feature, or documentation contract change. Do not reopen completed cleanup/A–E/provenance/crash-residue audits without new evidence.

## Read first in the next chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```
