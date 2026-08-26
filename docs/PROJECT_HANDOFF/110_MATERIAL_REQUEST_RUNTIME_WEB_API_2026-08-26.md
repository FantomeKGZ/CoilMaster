# CoilMaster — Material Request runtime/Web API

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN**

## Реализовано

Material Request теперь подключён к production Web runtime после repair-intake recovery.

API:

```text
POST /api/material-requests
GET  /api/material-requests?repair_id=...
GET  /api/material-requests/item?material_request_id=...
GET  /api/material-requests/movements?material_request_id=...
GET  /api/material-requests/status?material_request_id=...
POST /api/material-requests/status
POST /api/material-requests/warehouse
```

Правила:

- при создании заявки Web принимает `repair_id`, а `client_id + motor_id` получает сервер из authoritative repair record;
- новый Material Request создаётся только для существующего OPEN ремонта;
- warehouse mutation требует `confirmed=true`;
- цена материала не принимается от Web, а читается из ACTIVE `MaterialLedger` state;
- Web не вызывает `confirmUsage()`/`adjustMaterial()` напрямую;
- ISSUE/RETURN/CORRECTION выполняются только через crash-safe `MaterialRequestWarehouseCoordinator`;
- stores/pending/coordinator recovery выполняются до регистрации Material Request routes;
- lifecycle transition остаётся отдельным explicit API;
- `RUN_COMPLETED` остаётся non-mutating.

Основные файлы:

```text
firmware/esp32/src/CM_MaterialRequestWeb.h/.cpp
firmware/esp32/src/CM_MaterialRequestRuntime.h/.cpp
firmware/esp32/src/CM_RepairRegistry.h
firmware/esp32/src/CM_RepairRegistryLookup.cpp
firmware/esp32/src/CM_RepairRegistryWeb.cpp
Tests/Web/check_material_request_warehouse_coordinator.js
```

## Verification

```text
CMP run 32926200712 / SUCCESS
ESP32 Build run 32926237400 / SUCCESS
runtime bootstrap ESP32 Build 32926105448 / SUCCESS
```

One-shot runtime patch run `32926132861` также SUCCESS; workflow после применения удалён.

## Next

Следующий domain block: immutable repair delivery event/store/API. Repair completion, payment и delivery остаются независимыми фактами. Delivery должен хранить exact `repair_id + client_id + motor_id + delivered_at`, не требовать нулевого баланса и не редактировать старую историю.
