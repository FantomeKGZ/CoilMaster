# CoilMaster — repair AS_RECEIVED snapshot foundation

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN / WRITE-INTEGRATION PENDING**

Этот checkpoint фиксирует следующий Phase-A block после `97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md`.

## Цель

Сохранить неизменяемое состояние двигателя/обмотки на момент приёмки ремонта, чтобы последующая перемотка или изменение текущей карточки двигателя не переписывали историю старого ремонта.

## Новый store

```text
/data/workshop/repair-as-received.ndjson
```

Добавлены:

```text
firmware/esp32/src/CM_RepairAsReceivedSnapshotStore.h
firmware/esp32/src/CM_RepairAsReceivedSnapshotStore.cpp
Tests/Web/check_repair_as_received_snapshot_contract.js
```

Implementation commits:

```text
d6e4652aa3ff56d89138519c4ce2d4983b06a394  contract
568beb9c34512c936685a985c360179a0c92fb76  persistence
5bba647c7346ad03b9af028365a9b2f1753c2874  regression
3d1bb7af3ac5b0f2124604b2ad29343cae78d9b5  CI wiring
```

## Snapshot identity

```text
snapshot_id
repair_id
client_id
motor_id
winding_version_id optional
captured_at
source_kind
```

Read-only copied state:

```text
motor name / manufacturer / model
phases / slot_count
WORKING program / repeat_target / conductors
STARTING presence / program / repeat_target / conductors
comment optional
```

## Append-only / integrity semantics

- store uses `FILE_APPEND` only;
- `snapshot_id` strictly increases;
- `repair_id` strictly increases in normal append flow;
- lookup by `repair_id` fails closed on duplicate evidence;
- WORKING/STARTING programs canonicalized/validated;
- absent STARTING forbids hidden starting fields;
- incomplete NDJSON without final newline is invalid;
- existing `repairs.ndjson` and `motors.ndjson` are not rewritten.

## Runtime/read integration

Read-side integration is now implemented separately in checkpoint:

```text
docs/PROJECT_HANDOFF/99_CRM_WINDING_LOOKUP_API_2026-08-25.md
```

Available read endpoint:

```text
GET /api/repairs/as-received?repair_id=...
```

Legacy repairs created before this subsystem return an explicit legacy/no-snapshot state instead of being falsely classified as corrupt.

## Write-side transaction requirement

Automatic snapshot creation during `POST /api/repairs` is intentionally still pending.

Do **not** implement it as two best-effort independent appends (`repair` then `snapshot`) because a power loss between them can create an accepted repair without immutable intake evidence.

The next write-side block must add fail-closed transaction/recovery semantics so a new repair cannot be treated as complete if AS_RECEIVED persistence is missing.

## Safety

Unchanged:

- physical START local-only;
- ESP32/Web does not control SSR;
- RUN_COMPLETED never auto-deducts material;
- current exact-spool contract remains until coordinated migration;
- historical run/repair evidence is never silently erased.

## Verification

```text
CMP #3147 / run 32846371316 / SUCCESS
CMP #3148 / run 32846419170 / SUCCESS
ESP32 Build #1448 / run 32846323653 / SUCCESS
```

Foundation is SOFTWARE GREEN. Release-critical write integration and backup/integrity coverage remain pending.
