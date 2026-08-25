# CoilMaster — Material Request warehouse pending transaction foundation

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN**

## Реализовано

Добавлен durable single-in-flight pending store:

```text
/data/workshop/material-request-warehouse.pending.json
/data/workshop/material-request-warehouse.pending.tmp
```

Files:

```text
firmware/esp32/src/CM_MaterialRequestWarehousePendingStore.h
firmware/esp32/src/CM_MaterialRequestWarehousePendingStore.cpp
```

Marker stores the exact operator-confirmed transaction intent and cost snapshot:

```text
transaction_ref
material_request_id
repair_id
warehouse_item_id
movement_kind
source_kind
correction_direction
quantity_milli_units
unit
unit_cost_minor
cost_amount_minor
currency
created_at
comment
source_session_id + source_run_id
material_class + wire diameter
```

Allowed transaction semantics:

```text
ISSUE
RETURN
CORRECTION + ADD|REMOVE
```

`RUN_WIRE` is restricted to `ISSUE`, `KG`, exact session/run provenance, CU/AL and diameter.

Persistence uses verified temp -> rename. A second transaction is rejected while the durable marker exists.

## Stable backup guard

Both pending paths are now recovery markers in `CM_BackupExportWeb.cpp`:

```text
material_request_warehouse_pending
material_request_warehouse_temp_present
```

A stable backup is therefore rejected while a warehouse/request transaction is unfinished or its temp marker is present.

The first one-shot patch workflow was invalid before jobs started and changed no production code. It was removed. A corrected guarded patch used exact pre-change blob:

```text
7184f644dcf877c1e6d87acda8ea8a84b9bd04a0
```

Guarded patch run:

```text
32861669436 / SUCCESS
```

Post-patch `CM_BackupExportWeb.cpp` blob:

```text
cedbc04f39244976c3c9d1c94eca8d499070c85f
```

The one-shot workflow was deleted after successful application.

## Regression / verification

Permanent regression:

```text
Tests/Web/check_material_request_warehouse_pending.js
```

Permanent CMP workflow step:

```text
Audit Material Request warehouse pending transaction
```

Evidence:

```text
implementation head dc73b39e6f7d202d75dae801f5e3413218ca3c0e
ESP32 Build run 32861148982 / SUCCESS
CMP run 32861149158 / SUCCESS
CMP with permanent pending regression run 32861266055 / SUCCESS
backup guarded patch run 32861669436 / SUCCESS
```

## Scope boundary

Checkpoint 108 closes durable pending persistence + stable-backup guard only. It does NOT yet claim the physical stock + movement two-phase coordinator is complete.

## Next

1. Implement `MaterialRequestWarehouseCoordinator` using movement-first + ledger-second transaction ordering.
2. Persist a deterministic `transaction_ref` into movement/ledger evidence so recovery is idempotent.
3. Recovery matrix:
   - neither durable side -> safe no-op/retry path;
   - movement only -> finish physical ledger mutation;
   - ledger only -> impossible ordering / fail-closed;
   - both -> clear pending marker.
4. Support explicit operator ISSUE/RETURN/CORRECTION without implicit lifecycle rewrite.
5. Only after coordinator GREEN expose bounded warehouse/request Web APIs.

`RUN_COMPLETED` remains non-mutating. Existing exact-spool production flow remains authoritative until coordinated migration.
