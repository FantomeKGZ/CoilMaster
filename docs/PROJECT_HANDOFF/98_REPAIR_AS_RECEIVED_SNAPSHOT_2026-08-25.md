# CoilMaster — repair AS_RECEIVED snapshot foundation

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **IMPLEMENTED / ESP32 BUILD PENDING**

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
d6e4652aa3ff56d89138519c4ce2d4983b06a394  feat(crm): add repair as-received snapshot contract
568beb9c34512c936685a985c360179a0c92fb76  feat(crm): persist repair as-received snapshots
5bba647c7346ad03b9af028365a9b2f1753c2874  test(crm): protect repair as-received snapshots
3d1bb7af3ac5b0f2124604b2ad29343cae78d9b5  ci(crm): audit repair as-received snapshots
```

## Snapshot identity

Каждая запись связывает:

```text
snapshot_id
repair_id
client_id
motor_id
winding_version_id optional
captured_at
source_kind
```

И хранит read-only copy значимых данных при поступлении:

```text
motor name / manufacturer / model
phases / slot_count
WORKING program / repeat_target / conductors
STARTING presence / program / repeat_target / conductors
comment optional
```

## Append-only / integrity semantics

- store записывается только через `FILE_APPEND`;
- `snapshot_id` должен строго возрастать;
- `repair_id` должен строго возрастать, что обеспечивает максимум один snapshot на новый repair в normal append flow;
- lookup по `repair_id` fail-closed при duplicate snapshot;
- WORKING program canonicalized через `CM_WindingProgramParser`;
- STARTING validation fail-closed;
- отсутствие STARTING запрещает скрытые starting fields;
- incomplete NDJSON без финального newline считается invalid;
- существующие `repairs.ndjson` и `motors.ndjson` не переписываются.

## Compatibility

Этот блок пока является persistence foundation. Он не меняет текущий create-repair endpoint и пока не начинает автоматически писать snapshot при создании ремонта.

Следующий integration block обязан:

1. инициализировать winding-version + AS_RECEIVED stores в runtime;
2. дать read API для current/latest winding version;
3. при создании нового ремонта атомарно/с fail-closed semantics сохранить AS_RECEIVED evidence;
4. предоставить read endpoint snapshot по `repair_id`;
5. включить оба stores в backup whitelist + integrity audit до release-critical использования.

## Safety

Не меняются:

- physical START только локальный;
- ESP32/Web не управляет SSR;
- RUN_COMPLETED ничего автоматически не списывает;
- текущий exact-spool contract действует до отдельной полной migration;
- historical run/repair evidence не удаляется и не переписывается автоматически.

## Verification

```text
CMP #3147 / run 32846371316 / SUCCESS
CMP #3148 / run 32846419170 / running/queued at checkpoint creation
ESP32 Build #1448 / run 32846323653 / running at checkpoint creation
```

Не объявлять блок полностью GREEN до SUCCESS ESP32 Build #1448 и CI run #3148.
