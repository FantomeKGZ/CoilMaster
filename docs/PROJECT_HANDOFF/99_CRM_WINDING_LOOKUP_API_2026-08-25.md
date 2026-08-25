# CoilMaster — CRM winding/as-received runtime lookup API

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN / READ-SIDE INTEGRATED**

Этот checkpoint фиксирует read/runtime integration для новых Phase-A stores.

## Реализация

Расширен `MotorWindingVersionStore`:

```text
appendLatestByMotorJson()
appendMotorPageJson()
MaxPageSize = 24
```

Добавлена инициализация store lifecycle в `RepairRegistryLookupWeb` после SD startup, без изменения `main.cpp`.

Read-only API:

```text
GET /api/motors/winding/latest?motor_id=...
GET /api/motors/winding/versions?motor_id=...&cursor=...&limit=...
GET /api/repairs/as-received?repair_id=...
```

## Legacy compatibility

Если у старого двигателя ещё нет versioned winding record:

```json
"versioned": false,
"legacy_motor_fallback_required": true
```

Если старый ремонт создан до AS_RECEIVED snapshot subsystem:

```json
"snapshot_present": false,
"legacy_repair_without_snapshot": true
```

Legacy records не объявляются corrupted только потому, что были созданы до новой схемы.

## Pagination / integrity

- winding history фильтруется exact `motor_id`;
- cursor основан на monotonic `winding_version_id`;
- page limit bounded;
- malformed or non-monotonic NDJSON fails closed;
- endpoints read-only;
- неизвестный `motor_id` / `repair_id` возвращает 404 через authoritative registry lookup.

## Commits

```text
d22f2292abd3e04abbbc98107392a0add8d0f67b  expose winding version read contract
aa0516a64977fdba566dfa15d50e78ede5ab2e25  implement latest/page reads
8d41e5029c6bafc2039ac0db061e3e9e2bdeea64  lookup web lifecycle/API contract
91b05c434203c4a8152298b18444e36037fd23d9  serve read-only endpoints
8f62519746dd62bec29cb5cf27864665c634ba3b  regression
16ff6f45642c54b034611f71b79430e39ee27ee2  CI wiring
```

## Verification

```text
ESP32 Build #1452 / run 32846834525 / SUCCESS
CMP #3154 / run 32846834557 / SUCCESS
CMP #3155 / run 32846874546 / SUCCESS
CMP #3156 / run 32846924439 / SUCCESS
```

CMP #3156 includes explicit `Audit CRM winding/as-received lookup API` SUCCESS.

## Write-side note

Автоматическая запись AS_RECEIVED при `POST /api/repairs` пока намеренно не подключена.

Причина: простой sequence `append repair -> append snapshot` имеет crash-window. Нельзя считать новый repair корректно созданным, если snapshot потерян после первого append.

Следующий write-side block должен ввести fail-closed transaction/recovery semantics, а не два независимых best-effort append.

## Safety / unchanged contracts

- physical START remains local-only;
- ESP32/Web never directly controls SSR;
- RUN_COMPLETED never auto-deducts material;
- exact-spool contract remains current until coordinated migration;
- legacy motor/repair history is not destructively rewritten.
