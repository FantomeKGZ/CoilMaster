# 86 — Workshop winding single-pass audit — 2026-08-25

## Scope

Stage-1 ESP32/storage performance cleanup after the GREEN winding persistence single-pass block.

`WorkshopPersistenceIntegrityAudit::check()` previously validated `/data/winding-runs/events.ndjson` in two full passes:

1. `WindingJournalQuery::validateAll()` for schema validation;
2. `WindingJournalTransitionAudit::validate()` for STARTED/COMPLETED transition integrity.

After block 85, `WindingPersistenceIntegrityAudit::check()` already performs the same schema + transition validation in one authoritative journal pass. The workshop audit now delegates winding validation to that existing single-pass audit.

## Preserved standalone integrity

The workshop audit remains broad and fail-closed. It still performs:

- workshop registry validation through `RepairRegistry`;
- `WarehousePersistenceIntegrityAudit::check(storage)`;
- `PersistentIdIntegrityAudit::check(storage)`;
- `WindingSessionPersistenceIntegrityAudit::check(storage)`;
- winding schema + transition validation through `WindingPersistenceIntegrityAudit::check(storage)`.

No physical START, SSR, UART, Hall, run completion, spool provenance, or manual wire write-off semantics changed.

## Deliberate non-change

`WindingSessionPersistenceIntegrityAudit` keeps its directory preflight before `WindingSessionStore::begin()`. That preflight must detect temporary/recovery artifacts before `begin()` can perform cleanup or recovery. It is a safety/read-only boundary and is not treated as an accidental duplicate scan.

## Regression / CI

- `Tests/Web/check_workshop_winding_single_pass.js`
- CMP workflow step: `Audit workshop winding single-pass contracts`

Required software verification:

- ESP32 Build SUCCESS on the implementation commit or descendant;
- CMP Protocol Tests SUCCESS on the CI commit or descendant;
- explicit `Audit workshop winding single-pass contracts` SUCCESS.

Hardware testing is not required for this repo-only optimization block.
