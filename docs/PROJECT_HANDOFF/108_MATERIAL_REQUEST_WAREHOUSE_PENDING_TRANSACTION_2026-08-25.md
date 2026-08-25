# CoilMaster — Material Request warehouse pending transaction foundation

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **FOUNDATION GREEN; backup guard integration pending**

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
```

## Important unfinished part

Stable backup must also treat both pending paths as recovery markers. A guarded one-shot workflow intended to patch `CM_BackupExportWeb.cpp` was rejected by GitHub before any job ran, so production backup code was NOT modified by that failed workflow. The workflow was deleted immediately.

Current `CM_BackupExportWeb.cpp` blob observed before attempted patch:

```text
7184f644dcf877c1e6d87acda8ea8a84b9bd04a0
```

Do not call the entire warehouse transaction coordinator complete until backup recovery guard and coordinator recovery semantics are implemented and verified.

## Next

1. Add the two pending paths to stable-backup recovery markers by a safe guarded change.
2. Implement `MaterialRequestWarehouseCoordinator` using movement-first + ledger-second transaction ordering.
3. Use `transaction_ref` evidence to make reboot recovery idempotent.
4. Only then expose explicit ISSUE/RETURN/CORRECTION Web APIs.

`RUN_COMPLETED` remains non-mutating. Existing exact-spool production flow remains authoritative until coordinated migration.
