# CoilMaster — immutable repair delivery store/API

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN**

## Реализовано

Delivery отделена от repair completion и от cash/payment state.

Durable journal:

```text
/data/workshop/repair-deliveries.ndjson
```

Immutable fields:

```text
delivery_id
repair_id
client_id
motor_id
delivered_at
comment? 
```

Rules:

- explicit `confirmed=true` required for delivery write;
- server derives exact `client_id + motor_id` from authoritative repair identity;
- repair must already be CLOSED;
- exactly one final delivery event is allowed per repair;
- payment/balance is not a persistence gate: a motor may be delivered with outstanding debt;
- old delivery evidence is never edited or deleted.

API:

```text
GET  /api/repairs/delivery?repair_id=...
POST /api/repairs/delivery
```

Production runtime is registered from `RepairRegistryWeb::begin()`.

## Backup / integrity

Backup export includes:

```text
repair-deliveries -> /data/workshop/repair-deliveries.ndjson
```

`RepairDeliveryIntegrityAudit` is read-only and validates:

- canonical/monotonic delivery IDs;
- exact repair/client/motor identity match;
- uniqueness per repair;
- repair CLOSED state;
- timestamp/comment shape.

Any inconsistency makes stable backup fail closed with `repair_delivery_unstable_or_invalid`.

## Main files

```text
firmware/esp32/src/CM_RepairDeliveryStore.h/.cpp
firmware/esp32/src/CM_RepairDeliveryWeb.h/.cpp
firmware/esp32/src/CM_RepairDeliveryIntegrityAudit.h/.cpp
firmware/esp32/src/CM_RepairRegistryWeb.cpp
firmware/esp32/src/CM_BackupExportWeb.cpp
Tests/Web/check_crm_backup_integrity.js
```

## Verification

```text
backup patch run 32926981925 / SUCCESS
CMP delivery regression 32927089256 / SUCCESS
CMP integrated verification 32927153693 / SUCCESS
ESP32 Build integrated verification 32927153696 / SUCCESS
```

## Next

Implement append-only cash/payment events and repair/client balance readers. Repair pricing remains the authoritative charge source; cash journal records money received and corrections only. Delivery remains independent of balance.
