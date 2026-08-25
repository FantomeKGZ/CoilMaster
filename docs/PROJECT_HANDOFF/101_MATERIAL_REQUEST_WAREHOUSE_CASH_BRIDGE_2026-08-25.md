# CoilMaster — Material Request as Warehouse ↔ Cash bridge

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **APPROVED DESIGN / IMPLEMENTATION IN PROGRESS**

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

## 3. Material Request identity / lifecycle

Production stores:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-status.ndjson
```

Identity минимум:

```text
material_request_id
repair_id
client_id
motor_id
initial_status=DRAFT
created_at
comment optional
```

Status lifecycle реализован append-only:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

`CLOSED` не означает физическое копирование в другой архив. Append-only/history остаётся на месте, а активные UI просто фильтруют closed requests.

Checkpoint 107 подтвердил lifecycle, missing-request semantics, backup/export и fail-closed integrity.

## 4. Material request lines / movements

Материалы поддерживают не только провод:

- Cu/Al wire;
- varnish/lacquer;
- wedges/sticks;
- insulation;
- bearings;
- sleeves;
- fasteners;
- другие пользовательские warehouse items.

Append-only movement store:

```text
/data/workshop/material-request-movements.ndjson
```

Movements:

```text
movement_kind = ISSUE | RETURN | CORRECTION
source_kind = MANUAL_MATERIAL | RUN_WIRE
unit = KG | L | PCS | M | M2
```

Минимум movement evidence:

```text
movement_id
material_request_id
repair_id
warehouse_item_id
movement_kind
quantity_milli_units
unit
unit_cost_minor
cost_amount_minor
currency
created_at
```

Для провода дополнительно exact winding provenance:

```text
source_session_id
source_run_id
material_class CU/AL
wire_diameter
```

Для лака/клиньев/etc run provenance не требуется.

## 5. Warehouse catalog / units

Existing `MaterialLedger` и `/data/materials/materials.ndjson` являются authoritative generic warehouse item catalog; второй каталог не создаётся.

Canonical request mapping реализован:

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

`MaterialLedger::loadActiveMaterialState()` предоставляет единый fail-closed lookup unit/stock/price/currency.

## 6. Wire accounting integration

Целевой wire issue:

```text
RUN_COMPLETED
  -> НИЧЕГО автоматически не списывает
  -> оператор вручную указывает фактический расход
  -> warehouse ISSUE movement
  -> movement привязан к material_request_id + exact source_session_id + source_run_id
```

`spool_id` может остаться optional inventory metadata после полной migration. До полной migration текущий exact-spool backend остаётся действующим; нельзя частично отключать старые safety checks.

## 7. Warehouse responsibilities

Склад отвечает за:

- warehouse item catalog;
- physical quantity/current stock;
- bounded units;
- purchase/accounting cost;
- ISSUE/RETURN/CORRECTION movements;
- material request allocation;
- stock integrity/audit.

Склад **не** хранит клиентские платежи и не решает, сколько клиент уже оплатил.

## 8. Costing/Cash responsibilities

Costing читает подтверждённые складом quantities и их cost snapshot.

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

Навигация должна быть двусторонней:

```text
payment -> repair -> material request -> warehouse movements
material request -> repair -> client -> cash/payment history
```

## 9. Archive semantics

После завершения ремонта:

- material request status становится `CLOSED`;
- движения не переписываются;
- request остаётся доступной через client/motor/repair history;
- active warehouse/cash UI скрывают closed по умолчанию, но дают фильтр `Архив`.

## 10. Corrections and returns

После ISSUE нельзя молча изменить quantity старой позиции.

```text
ISSUE Cu 1.00 = -2.40 kg
RETURN Cu 1.00 = +0.30 kg
actual consumed = 2.10 kg
```

Ошибочная финансовая операция аналогично исправляется отдельной cash correction/refund event, а не rewrite.

## 11. Current implementation status

SOFTWARE GREEN:

```text
103 Material Request identity + movement schema
104 CRM backup/export + integrity
105 MaterialLedger serialization fix
106 MaterialLedger adapter + active item lookup
107 Material Request lifecycle + lifecycle backup/integrity
```

Latest lifecycle evidence:

```text
head a960999b040afbdd7c48bbde08763e042408a2e8
CMP run 32860049965 / SUCCESS
ESP32 Build run 32860049946 / SUCCESS
```

## 12. Current implementation order

Immediate NEXT:

1. crash-safe transaction coordinator for explicit operator ISSUE/RETURN/CORRECTION;
2. durable pending/recovery marker coupling physical MaterialLedger mutation and immutable movement evidence;
3. fail-closed lifecycle gates around warehouse mutation;
4. bounded runtime/Web Material Request APIs;
5. delivery event/store/API;
6. payment/correction store/API;
7. Motor Web;
8. Client Web;
9. coordinated spool -> material-request wire migration;
10. costing/cash integration;
11. archive/navigation/analytics foundations;
12. full software + hardware acceptance.

Material request subsystem строится **до cash.html**, иначе кассу пришлось бы переделывать после warehouse integration.

## 13. Safety / integrity invariants

Не менять:

- `RUN_COMPLETED` never auto-deducts material;
- physical stock movement requires explicit operator action;
- physical stock mutation and immutable request evidence must recover atomically/fail-closed;
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

Каждый крупный persistence/API блок получает numbered checkpoint с exact commits + CI evidence.
