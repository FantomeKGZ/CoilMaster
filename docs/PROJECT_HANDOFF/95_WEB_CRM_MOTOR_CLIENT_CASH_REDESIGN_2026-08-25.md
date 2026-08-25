# CoilMaster — Web/CRM motor, client, repair, warehouse and cash redesign

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
-> MATERIAL REQUEST
   -> warehouse ISSUE/RETURN/CORRECTION
   -> costing/material valuation
-> CASH / PAYMENTS / BALANCE
-> repair completion
-> DELIVERED_TO_CLIENT
-> archived/read-only history
```

Ключевое разделение ответственности:

```text
СКЛАД = физические остатки и движение материалов
КАССА = деньги, начисления, оплаты и баланс
ЗАЯВКА МАТЕРИАЛОВ = связующий документ repair ↔ warehouse ↔ costing/cash
```

Подробный material-request design:

```text
docs/PROJECT_HANDOFF/101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

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

Каждая version поддерживает отдельные WORKING и optional STARTING programs, repeat targets, coil pitch и multi-conductor data. Реальные комбинации вроде `0.95 + 1.00`, `0.80 x 3`, `0.71 x 2 + 0.80` должны поддерживаться без привязки к одной бухте.

Для 3-фазного двигателя STARTING обычно отсутствует.

## 4. Motor Web

### `/desktop/motors.html`

Catalog-only в стиле `/desktop/arduino-windings.html`: компактный список, поиск, bounded paging, фильтры, быстрый переход в карточку.

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

### `/desktop/motor-new.html`

Отдельная страница создания motor master. Другие страницы оставляют только ссылку `+ Добавить двигатель`.

### `/desktop/motor-details.html`

Рабочая карточка двигателя:

- identity/passport data;
- current winding version;
- WORKING / STARTING;
- conductor data;
- version history;
- before/after comparison;
- repair history;
- material requests history;
- direct `Отправить рабочую/пусковую на станок` actions.

JOB creation никогда не означает physical START. Physical START остаётся только локальным.

## 5. AS_RECEIVED snapshot

При создании нового ремонта фиксируется immutable copy состояния двигателя/обмотки на момент приёмки. Старый ремонт продолжает показывать `как поступил`, даже если текущая motor card изменилась.

Snapshot минимум:

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

- identity/contacts/notes;
- физические двигатели клиента;
- ремонты;
- material requests;
- начислено / оплачено / баланс;
- payment history;
- accepted/completed/delivered dates;
- ссылки client -> motor -> repair -> material request и обратно.

Не встраивать `client_id` в motor master как вечного владельца. Связь клиента с мотором следует из repair/history semantics.

## 7. Material Request — связка склада и кассы

Главный owner заявки — `repair_id`; для exact provenance также сохраняются `client_id + motor_id`.

Target lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Заявка содержит/агрегирует фактические складские движения:

```text
ISSUE
RETURN
CORRECTION
```

Материалы: провод Cu/Al, лак, клинья/палочки, изоляция, подшипники и любые другие warehouse items.

После ISSUE исходную строку нельзя молча переписать или удалить. Возврат/исправление — отдельный append-only movement.

После CLOSED заявка не копируется в отдельный файл-архив; она остаётся доступной через repair/client/motor history, а active UI просто фильтрует closed requests.

## 8. Склад

Склад отвечает только за physical inventory:

- item catalog;
- quantity/current stock;
- bounded unit set (`kg/g/l/ml/pcs/...`);
- accounting/purchase cost;
- ISSUE/RETURN/CORRECTION;
- привязку movements к material request;
- inventory integrity/audit.

Склад не хранит клиентские платежи.

## 9. Wire accounting migration

Ранее одобренный уход от mandatory exact `spool_id` теперь реализуется как часть material-request architecture.

Target future wire issue:

```text
RUN_COMPLETED
-> НИЧЕГО автоматически не списывает
-> оператор вводит фактический расход
-> warehouse ISSUE movement
-> material_request_id
-> exact source_session_id + source_run_id
-> material class CU/AL + actual weight
```

`spool_id` может остаться optional inventory metadata после полной migration.

КРИТИЧНО: текущий production backend/finalization пока использует exact spool identity. Нельзя убрать только Web selector. Migration должна согласованно обновить job metadata, writeoff, material request movement, costing, finalization, backup/integrity, reports, Web и tests.

## 10. Costing vs Cash

Costing читает подтверждённые warehouse movements/material request и считает фактическую себестоимость.

Полезно хранить отдельно:

```text
cost_amount = себестоимость мастерской
charge_amount = сумма, начисленная клиенту
```

Cash хранит только financial events:

```text
repair/client charge
payment
correction/refund
balance
```

`/desktop/cash.html` показывает:

```text
Дата | Клиент | Двигатель | Ремонт | Заявка | Начислено | Оплачено | Остаток | Статус
```

Поддержать partial/multiple payments, debt, overpayment и append-only corrections.

## 11. Repair lifecycle / delivery

`CLOSED` ремонта и физическая выдача клиенту — разные события.

Append-only delivery evidence:

```text
repair_id
client_id
motor_id
delivered_at
comment optional
```

Долг не hard-block выдачи. UI предупреждает и требует explicit operator confirmation.

## 12. Backward compatibility

- старые `motors.ndjson` остаются читаемыми;
- legacy `coil_program + repeat_target` синтезируется как legacy/current WORKING;
- старые repairs остаются читаемыми;
- repair без нового snapshot получает явный legacy status;
- никаких destructive rewrite historical stores;
- new history/event/version stores append-only where practical;
- каждый release-critical store обязан войти в backup whitelist + integrity validation.

## 13. Safety invariants — never weaken

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts wire/material;
- physical warehouse ISSUE requires explicit operator action;
- run-linked wire ISSUE retains exact `source_session_id + source_run_id`;
- cancellation/operator abort never erases immutable run/history evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## 14. Phase A implementation status

### GREEN — winding version schema/persistence
`97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md`

### GREEN — AS_RECEIVED persistence foundation
`98_REPAIR_AS_RECEIVED_SNAPSHOT_2026-08-25.md`

### GREEN — runtime/read API
`99_CRM_WINDING_LOOKUP_API_2026-08-25.md`

### GREEN — repair intake pending transaction foundation
`100_REPAIR_INTAKE_TRANSACTION_FOUNDATION_2026-08-25.md`

### INTEGRATED / verification pending — transactional POST `/api/repairs`

Current integrated commit:

```text
92523c7c6f4c8af8c71a63c4178a4b1e41953f19
feat(crm): make repair intake snapshot transactional
```

The integration uses `RepairIntakeCoordinator`; it must not be declared GREEN until a normal connector-triggered ESP32 Build + CMP regression run succeeds.

## 15. Updated implementation order

1. Finish transactional repair intake verification.
2. Add winding/snapshot/intake stores to backup whitelist + integrity audit.
3. Implement **Material Request schema + movement store + warehouse item/unit contract** from checkpoint 101.
4. Add delivery event/store/API.
5. Add payment/correction store/API.
6. Implement Motor Web (`motor-new`, new catalog, motor card).
7. Implement Client Web (`client-new`, catalog, client card).
8. Coordinated spool -> material-request wire migration.
9. Costing/material request integration.
10. `cash.html` + payment integration.
11. Archive/navigation/analytics foundations.
12. Desktop/mobile alignment, regressions, backup/restore audit.
13. Repeat full hardware E2E acceptance on final contracts.

## 16. Documentation discipline

После каждого meaningful implementation block обновлять своевременно:

```text
this file
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md when material-request design/implementation changes
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
00_READ_FIRST.md when entrypoint/read order changes
```

Для крупных stores/API создавать numbered checkpoint с exact commit SHA и CI evidence.
