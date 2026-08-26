# Активная работа и следующие шаги

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 133

```text
97-116 CRM / Material Request / Motor / Client / Cash
117-127 exact-spool bridge + atomic RUN_WIRE + accounting/provenance convergence
128-130 obsolete direct writeoff API/types/implementations narrowed then removed
131 repair existence lookup made explicitly fail-closed
132 spool identity and wire catalogue lookups made explicitly fail-closed
133 public price lookup now requires explicit configured output
```

Verified checkpoint-133 evidence:

```text
fe0992219e50800e0dbf181b365d5788f76fc445
70f987ba58becb1d43ff744bc4ceb02c9cedc0bf
ESP32 Build #1580  32966647349 / SUCCESS
CMP Tests #3584    32966706823 / SUCCESS
```

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Public fail-closed warehouse lookup forms:

```text
repairExists(repairId, found)
loadActiveSpoolIdentity(spoolId, identity, found)
loadKnownWireDiameters(type, items, capacity, count)
loadWarehousePrice(price, configured)
```

## Current active queue — NDJSON runtime scan optimization

1. Inspect `WarehouseMovementIntegrityAudit` and `WarehouseStore::readMovements` to confirm whether `/api/warehouse/summary` currently performs two complete movement-log passes.
2. If so, expose validated summary aggregation from the authoritative integrity pass or otherwise remove the duplicate pass without reducing schema/provenance validation.
3. Check the same pattern for spool scans only after movement optimization is compile/test proven.
4. Keep diagnostics read-only and keep automatic cleanup/rotation/deletion disabled.
5. Do not migrate to a DB prematurely.
6. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
