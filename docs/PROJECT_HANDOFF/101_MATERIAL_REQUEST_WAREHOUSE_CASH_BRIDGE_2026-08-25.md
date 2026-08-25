# CoilMaster — Material Request as Warehouse ↔ Cash bridge

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **APPROVED DESIGN / IMPLEMENTATION NEXT**

Этот checkpoint дополняет authoritative CRM design `95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md`.

## 1. Главная идея

Разделить ответственность:

```text
СКЛАД = физические остатки и фактическое движение материалов
КАССА = деньги, начисления, оплаты, баланс
ЗАЯВКА МАТЕРИАЛОВ = связующий документ между ремонтом, складом и кассой
```

Главным владельцем заявки является `repair_id`, а не клиент напрямую. Через ремонт однозначно доступны `client_id` и `motor_id`, но для удобства/immutability заявка также сохраняет exact provenance `repair_id + client_id + motor_id`.

## 2. Целевой поток

```text
CLIENT
  -> MOTOR
  -> REPAIR
  -> MATERIAL REQUEST (DRAFT)
       -> warehouse issue / return / correction
       -> actual physical quantities fixed
       -> costing reads material valuation
       -> cash receives priced repair
  -> repair completed
  -> payments
  -> delivered to client
  -> request CLOSED/archive-visible
```

Никакая позиция материалов не должна исчезать после фактической выдачи со склада. Исправления оформляются отдельными RETURN/CORRECTION movements.

## 3. Material Request identity

Предлагаемый production store:

```text
/data/workshop/material-requests.ndjson
```

Header/event identity минимум:

```text
material_request_id
repair_id
client_id
motor_id
status
created_at
closed_at optional
comment optional
```

Status lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

`CLOSED` не означает физическое копирование в другой архив. Append-only/history остаётся на месте, а активные UI просто фильтруют closed requests.

## 4. Material request lines / movements

Материалы должны поддерживать не только провод:

- Cu/Al wire;
- varnish/lacquer;
- wedges/sticks;
- insulation;
- bearings;
- sleeves;
- fasteners;
- другие пользовательские warehouse items.

Рекомендуемый append-only movement/event store:

```text
/data/workshop/material-request-movements.ndjson
```

Минимум:

```text
movement_id
material_request_id
repair_id
warehouse_item_id or canonical material identity
movement_kind = ISSUE | RETURN | CORRECTION
quantity
unit
unit_cost_snapshot
created_at
```

Для провода дополнительно сохраняется exact winding provenance, когда расход относится к реальному запуску:

```text
source_session_id optional
source_run_id optional
material_class CU/AL optional
wire_diameter optional
```

Для лака/клиньев/etc run provenance не требуется.

## 5. Wire accounting integration

Утверждённый ранее переход от mandatory spool selection теперь должен входить в material-request architecture, а не существовать как отдельный параллельный workflow.

Целевой wire issue:

```text
RUN_COMPLETED
  -> НИЧЕГО автоматически не списывает
  -> оператор вручную указывает фактический расход
  -> warehouse ISSUE movement
  -> movement привязан к material_request_id + exact source_session_id + source_run_id
```

`spool_id` может остаться optional inventory metadata, но не должен быть главным обязательным provenance после полной migration.

До полной migration текущий exact-spool backend остаётся действующим. Нельзя частично отключать старые safety checks.

## 6. Warehouse responsibilities

Склад должен отвечать за:

- warehouse item catalog;
- physical quantity/current stock;
- unit (`kg`, `g`, `l`, `ml`, `pcs`, etc. bounded set);
- purchase/accounting cost;
- ISSUE/RETURN/CORRECTION movements;
- material request allocation;
- stock integrity/audit.

Склад **не** хранит клиентские платежи и не решает, сколько клиент уже оплатил.

## 7. Costing/Cash responsibilities

Costing читает уже подтверждённые складом quantities и их cost snapshot.

Для каждой позиции полезно различать:

```text
cost_amount = фактическая себестоимость мастерской
charge_amount = сумма, включённая клиенту
```

Cash хранит только финансовые события:

```text
repair price / charge
payments
corrections/refunds
balance
```

Cash page должна уметь перейти:

```text
payment -> repair -> material request -> warehouse movements
```

и обратно:

```text
material request -> repair -> client -> cash/payment history
```

## 8. Bidirectional navigation

Client card:

```text
client
 -> motor
 -> repair
 -> material request
 -> payments
 -> delivery
```

Material request card:

```text
request
 -> repair
 -> motor
 -> client
 -> warehouse movements
 -> costing/cash
```

Motor card:

```text
motor
 -> repair history
 -> each repair material request
```

## 9. Archive semantics

После завершения ремонта:

- material request status становится `CLOSED`;
- движения не переписываются;
- request остаётся доступной через client/motor/repair history;
- active warehouse/cash UI скрывают closed по умолчанию, но дают фильтр `Архив`.

## 10. Corrections and returns

После ISSUE нельзя молча изменить quantity старой позиции.

Пример:

```text
ISSUE Cu 1.00 = -2.40 kg
RETURN Cu 1.00 = +0.30 kg
actual consumed = 2.10 kg
```

Ошибочная финансовая операция аналогично исправляется отдельной cash correction/refund event, а не rewrite.

## 11. Analytics enabled by this model

После реализации можно без дополнительной миграции получать:

- расход Cu/Al за период;
- расход лака/клиньев/etc;
- закупочную стоимость материалов;
- charge клиенту по материалам;
- gross margin по ремонту;
- прибыль/выручка по клиенту;
- наиболее расходуемые материалы;
- ready-to-order stock warnings;
- unpaid repairs;
- completed-but-not-delivered repairs.

## 12. Implementation order change

Новый порядок внутри Phase A:

1. завершить transactional repair intake + AS_RECEIVED;
2. backup/integrity для winding/snapshot/intake stores;
3. **material request schema + request movements + warehouse item/unit contract**;
4. delivery event;
5. payment/correction store;
6. Motor Web;
7. Client Web;
8. coordinated spool -> material-request wire migration;
9. costing/cash integration;
10. archive/navigation/analytics foundations;
11. full software + hardware acceptance.

Material request schema создаётся **до cash.html**, иначе кассу пришлось бы переделывать после warehouse integration.

## 13. Safety / integrity invariants

Не менять:

- `RUN_COMPLETED` never auto-deducts material;
- physical stock movement requires explicit operator action;
- run-linked wire ISSUE сохраняет exact `source_session_id + source_run_id`;
- no silent rewrite/delete of issued movements;
- corrections/returns append-only;
- restore fail-closed;
- no automatic production NDJSON truncation;
- Arduino physical START / SSR ownership invariants unchanged.

## 14. Documentation discipline

Синхронно обновлять:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
00_READ_FIRST.md
```

При начале кода material-request subsystem создать следующий numbered implementation checkpoint с exact commits + CI evidence.
