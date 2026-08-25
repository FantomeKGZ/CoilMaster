# CoilMaster — Web/CRM motor, client, repair and cash redesign

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **APPROVED DESIGN / PHASE A IMPLEMENTATION IN PROGRESS**

Этот checkpoint — authoritative design текущего Web/CRM этапа. История конкретных implementation blocks фиксируется numbered checkpoints 97+.

## 1. Целевой рабочий поток

```text
КЛИЕНТ
-> его физические ДВИГАТЕЛИ
-> РЕМОНТ
-> immutable AS_RECEIVED snapshot
-> WORKING / STARTING winding data
-> winding job(s)
-> resulting winding version
-> COSTING
-> PAYMENTS / BALANCE
-> repair completion
-> DELIVERED_TO_CLIENT
```

Каталоги становятся browse/search-oriented. Создание клиента/двигателя переносится на отдельные страницы. Карточки клиента и двигателя становятся основными рабочими экранами.

## 2. Stable baseline / branch rule

До начала CRM redesign зафиксирован stable pre-CRM commit:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> этот commit
stable-2026-08-25-pre-crm-redesign -> этот commit
```

Вся новая работа идёт только в `cmp-protocol-v1`. `main` не использовать как source и не двигать без отдельного согласованного stable checkpoint.

## 3. Двигатель / winding versions

Один физический двигатель = один `motor_id`.

Перемотка Al -> Cu или последующая перемотка не создаёт новый motor master. Вместо этого:

```text
motor_id
  -> winding version 1: AS_RECEIVED / ORIGINAL
  -> winding version 2: REWOUND / CURRENT
  -> winding version N
```

Каждая version поддерживает:

```text
WORKING
  coil_program
  repeat_target
  coil_pitch optional
  conductors[]

STARTING optional
  coil_program
  repeat_target
  coil_pitch optional
  conductors[]
```

Для 3-фазного двигателя STARTING обычно отсутствует.

Multi-conductor должен покрывать реальные комбинации:

```text
0.95 + 1.00
0.80 x 3
0.71 x 2 + 0.80
```

## 4. Motor Web

### `/desktop/motors.html`

Catalog-only в визуальном/рабочем стиле `/desktop/arduino-windings.html`.

Основные поля строки:

```text
Название
Фазы
Рабочая программа
Повторов рабочей
Пусковая программа
Повторов пусковой
Провод/материал кратко
Открыть
```

Фильтры: identity, phases, slots, Cu/Al, starting presence, winding program, power where available.

### `/desktop/motor-new.html`

Отдельная страница создания motor master. Другие страницы больше не содержат большую inline motor form; оставляют ссылку `+ Добавить двигатель`.

После создания — переход в `motor-details.html?motor_id=...`.

### `/desktop/motor-details.html`

Рабочая карточка:

- identity/passport data;
- current winding version;
- WORKING;
- STARTING;
- conductor data;
- version history;
- before/after comparison;
- repair history;
- source/notes;
- direct actions.

Кнопки:

```text
Отправить рабочую на станок
Отправить пусковую на станок
```

Они создают JOB, но никогда не выполняют physical START. Physical START остаётся только локальным.

## 5. AS_RECEIVED snapshot

При создании нового ремонта должна фиксироваться immutable copy состояния двигателя/обмотки на момент приёмки.

Старый ремонт обязан продолжать показывать состояние `как поступил`, даже если текущая motor card позже обновилась.

Snapshot включает минимум:

```text
repair_id + client_id + motor_id
optional winding_version_id
captured_at / source_kind
motor identity
phases / slots
WORKING program/repeats/conductors
STARTING presence/program/repeats/conductors
```

Новый repair не должен считаться корректно созданным, если обязательный AS_RECEIVED evidence потерян из-за crash/power loss.

## 6. Клиенты

### `/desktop/clients.html`

Catalog-only. Поиск минимум по имени, телефону, ID.

### `/desktop/client-new.html`

Отдельное создание клиента.

### `/desktop/client-details.html`

Показывает:

- имя/телефон/заметки;
- двигатели, которые клиент привозил;
- ссылки на physical motor cards;
- ремонты;
- open / completed-not-delivered;
- начислено / оплачено / баланс;
- payment history;
- accepted/completed/delivered dates.

Не встраивать постоянный `client_id` в motor master как вечного владельца. Связь клиента с мотором следует из repair/history semantics.

## 7. Repair lifecycle / delivery

`CLOSED` ремонта и физическая выдача клиенту — разные события.

Target:

```text
repair completed
ready for delivery
delivered to client
```

Добавить append-only delivery evidence:

```text
repair_id
client_id
motor_id
delivered_at
comment optional
```

Долг не hard-block выдачи. UI предупреждает и требует explicit operator confirmation.

## 8. Costing vs cash

Существующий costing отвечает за:

- себестоимость;
- провод/materials;
- labour;
- client price;
- margin/loss;
- pricing revision history.

Cash/payments — отдельная подсистема.

Target `/desktop/cash.html`:

```text
Дата | Клиент | Двигатель | Ремонт | Начислено | Оплачено | Остаток | Статус
```

Payment storage append-only/correction-based:

```text
payment/correction id
client_id
repair_id
amount
timestamp
```

Поддержать partial/multiple payments, debt, overpayment, aggregated client balance.

## 9. Wire accounting migration

Пользователь одобрил уход основного workflow от обязательной exact `spool_id`.

Target future contract:

```text
source_session_id + source_run_id
material class CU/AL
actual consumed weight
manual confirmation
```

`RUN_COMPLETED` никогда ничего автоматически не списывает.

КРИТИЧНО: migration ещё не завершена. Текущий backend/finalization использует exact spool identity. Нельзя убрать только Web selector.

Migration должна согласованно обновить:

- linked job creation;
- immutable job metadata;
- writeoff API/storage;
- costing;
- finalization;
- backup/integrity;
- reports/history;
- Web;
- tests/docs.

Spool inventory UI можно сохранить как optional interface.

## 10. Backward compatibility

- старые `motors.ndjson` остаются читаемыми;
- legacy `coil_program + repeat_target` синтезируется как legacy/current WORKING до upgrade;
- старые repairs остаются читаемыми;
- repair без нового snapshot получает явный legacy status, а не ложную corruption classification;
- никаких destructive rewrite historical stores;
- новые history/event/version stores предпочтительно append-only;
- каждый release-critical store обязан войти в backup whitelist + integrity validation.

## 11. Safety invariants — never weaken

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts wire/material;
- cancellation/operator abort never erases immutable run/history evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## 12. Phase A implementation status

### GREEN — winding version schema/persistence

Checkpoint:

```text
97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md
```

Implemented `/data/workshop/motor-winding-versions.ndjson`, WORKING/STARTING, predecessor, repair linkage, multi-conductor.

### GREEN — AS_RECEIVED persistence foundation

Checkpoint:

```text
98_REPAIR_AS_RECEIVED_SNAPSHOT_2026-08-25.md
```

Implemented `/data/workshop/repair-as-received.ndjson` and immutable snapshot contract.

### GREEN — runtime/read API

Checkpoint:

```text
99_CRM_WINDING_LOOKUP_API_2026-08-25.md
```

Read-only endpoints:

```text
GET /api/motors/winding/latest
GET /api/motors/winding/versions
GET /api/repairs/as-received
```

Verified through ESP32 Build #1452 and CMP #3154-#3156.

### IN PROGRESS — repair intake transaction/recovery

Added foundation:

```text
/data/workshop/repair-intake.pending.json
/data/workshop/repair-intake.pending.tmp
CM_RepairIntakePendingStore
```

Purpose: close power-loss window between new repair append and mandatory AS_RECEIVED snapshot append.

Required semantics:

1. durable pending marker before repair append;
2. if crash before repair exists -> pending may be discarded as uncommitted;
3. if repair exists but snapshot missing -> recover exact snapshot from marker/source version before normal operation;
4. if repair+snapshot both exist -> clear stale marker;
5. ambiguous/corrupt state -> fail closed.

This block must not be declared GREEN before CI/build evidence.

## 13. Remaining implementation order

1. Finish repair intake transaction/recovery and connect `POST /api/repairs`.
2. Add winding/snapshot/pending stores to backup whitelist + integrity audit.
3. Add delivery event/store/API.
4. Add payment/correction store/API.
5. Implement `motor-new.html`.
6. Redesign `motors.html`.
7. Expand `motor-details.html` and direct WORKING/STARTING job send.
8. Implement `client-new.html`, catalog-only `clients.html`, `client-details.html`.
9. Repair delivery lifecycle UI.
10. Coordinated spool -> material+weight migration.
11. `cash.html` and payment integration.
12. Desktop/mobile navigation alignment, regressions, backup/restore audit.
13. Repeat full hardware E2E acceptance on final contracts.

## 14. Documentation discipline

После каждого meaningful implementation block обновлять своевременно:

```text
this file
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md when transfer state changes
00_READ_FIRST.md when entrypoint/read order changes
```

Для крупных stores/API создавать numbered checkpoint с exact commit SHA и CI evidence.
