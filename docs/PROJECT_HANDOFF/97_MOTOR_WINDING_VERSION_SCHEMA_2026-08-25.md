# CoilMaster — motor winding version schema foundation

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN / FOUNDATION ONLY**

Этот checkpoint фиксирует первый implementation block Phase A из `95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md`.

## Цель блока

Добавить backward-compatible основу для версионных обмоток двигателя без destructive rewrite существующего `/data/workshop/motors.ndjson`.

Legacy motor master остаётся читаемым. Новые обмоточные версии хранятся в отдельном append-only store:

```text
/data/workshop/motor-winding-versions.ndjson
```

## Реализация

Добавлены:

```text
firmware/esp32/src/CM_MotorWindingVersionStore.h
firmware/esp32/src/CM_MotorWindingVersionStore.cpp
```

Implementation commits:

```text
a4895a6d058a56cd3041573b38d6ec808196cc99  feat(crm): add motor winding version contract
2513d5392840fc739f402ce754f9543996086bc3  feat(crm): persist motor winding versions
```

## Contract

Один физический двигатель продолжает иметь один `motor_id`.

Новая запись version store содержит:

```text
winding_version_id
motor_id
previous_version_id optional
source_repair_id optional
version_kind
created_at
WORKING role
STARTING role optional
comment optional
```

WORKING обязателен для новой winding version. STARTING может отсутствовать, что покрывает обычный 3-фазный двигатель.

Для каждого role поддерживаются:

```text
coil_program
repeat_target
coil_pitch optional
conductors
```

## Multi-conductor representation

В runtime contract разрешено до 4 conductor components на role.

Каждый component:

```text
diameter_hundredths_mm
strand_count
material_class = CU | AL
```

Append-only NDJSON хранит conductor list в bounded canonical string, например:

```text
CU:95x1+CU:100x1
CU:80x3
AL:71x2+AL:80x1
```

Это позволяет представлять реальные случаи `0.95 + 1.00`, `0.80 x 3` и аналоги без JSON-array parser overhead на ESP32.

## Compatibility

- `motors.ndjson` не переписывается;
- старые `coil_program + repeat_target` остаются legacy/current WORKING source до явного upgrade;
- новые winding versions могут ссылаться на predecessor через `previous_version_id`;
- version может быть связана с ремонтом через `source_repair_id`;
- текущий exact-spool production contract этим блоком не меняется;
- RUN_COMPLETED и physical START invariants не меняются.

## Fail-closed behavior

Store:

- требует WORKING role;
- canonicalizes winding programs через `CM_WindingProgramParser`;
- принимает только `CU` / `AL` conductor classes;
- запрещает zero diameter / zero strands;
- ограничивает conductor count;
- проверяет append-only monotonic `winding_version_id`;
- требует newline-complete NDJSON before append/read;
- при write failure переводит store в not-ready.

## Verification

На commit `2513d539...`:

```text
CMP Protocol Tests #3137 / run 32844995517 / SUCCESS
ESP32 Build #1446 / run 32844995460 / SUCCESS
```

Блок считается software GREEN только как schema/persistence foundation. Store ещё не подключён к runtime/API и ещё не release-critical.

## Следующие шаги

1. Добавить regression contract для schema/canonical conductor format.
2. Подключить store lifecycle к ESP32 runtime.
3. Добавить read/latest/page API для motor card/catalog.
4. До release-critical использования добавить store в backup whitelist + integrity audit.
5. Затем перейти к immutable repair `AS_RECEIVED` snapshot contract.
