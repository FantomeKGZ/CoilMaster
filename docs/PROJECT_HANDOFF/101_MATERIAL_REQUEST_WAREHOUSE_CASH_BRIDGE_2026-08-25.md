# CoilMaster — Material Request as Warehouse ↔ Cash bridge

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**  
Статус: **APPROVED DESIGN / IMPLEMENTATION IN PROGRESS**

Этот документ дополняет authoritative CRM design `95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md`.

## 1. Главная идея

Разделить ответственность:

```text
СКЛАД = физические остатки и фактическое движение материалов
КАССА = деньги, начисления, оплаты, баланс
ЗАЯВКА МАТЕРИАЛОВ = связующий документ между ремонтом, складом и кассой
```

Главным владельцем заявки является `repair_id`. Для immutable provenance заявка также хранит exact `repair_id + client_id + motor_id`.

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

Фактические движения append-only. После ISSUE старая запись не переписывается; исправления оформляются RETURN/CORRECTION.

## 3. Material Request identity / lifecycle

Production stores:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-status.ndjson
```

Identity:

```text
material_request_id
repair_id
client_id
motor_id
initial_status=DRAFT
created_at
comment optional
```

Append-only lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

`CLOSED` не переносит запись в другой архив; история остаётся на месте.

## 4. Material Request movements

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

Основное evidence:

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

Для `RUN_WIRE` дополнительно обязательно exact winding provenance:

```text
source_session_id
source_run_id
material_class = CU | AL
wire_diameter
```

## 5. Warehouse catalog / units

Existing `MaterialLedger` at `/data/materials/materials.ndjson` остаётся authoritative generic warehouse item catalog; второй каталог не создаётся.

Canonical mapping:

```text
KG  -> GRAM x1000
L   -> MILLILITRE x1000
PCS -> PIECE x1
M   -> METRE x1
M2  -> SQUARE_METRE x1
```

`MaterialLedger::loadActiveMaterialState()` предоставляет authoritative lookup unit/stock/price/currency.

Checkpoint 119 добавил backward-compatible structured wire metadata:

```text
wire_type = CU | AL
diameter_hundredths_mm = exact diameter
wire metadata valid only with unit = GRAM
```

Старые generic/non-wire records без этой пары остаются валидными.

## 6. Physical spool ↔ MaterialLedger bridge

Текущая система исторически имеет две разные inventory identities:

```text
physical wire spool domain -> spool_id
MaterialLedger catalog      -> warehouse_item_id
```

Checkpoints 118–120 создали безопасный мост:

```text
/data/warehouse/spool-material-bridges.ndjson
spool_id <-> warehouse_item_id + CU/AL + exact diameter
```

Properties:
- append-only identity evidence;
- bounded duplicate/reference integrity;
- warehouse backup/export/integrity coverage;
- exact physical spool ↔ MaterialLedger `GRAM + CU/AL + diameter` cross-check;
- no stock mutation from bridge creation.

Production operator endpoint since checkpoint 120:

```text
POST /api/warehouse/spool-material-bridges
```

Request requires:

```text
spool_id
warehouse_item_id
confirm=1
linked_at
```

The server loads authoritative ACTIVE spool and ACTIVE MaterialLedger state. CU/AL and diameter are not accepted as caller authority; exact matching data is derived from the persisted records. Already-bridged spool fails closed.

## 7. Wire accounting integration

Target run-linked wire issue:

```text
RUN_COMPLETED
  -> НИЧЕГО автоматически не списывает
  -> оператор вручную подтверждает фактический расход
  -> crash-safe warehouse ISSUE
  -> material_request_id + exact source_session_id + source_run_id
  -> exact physical spool provenance retained through bridge
```

Current exact-spool backend/writeoff/finalization remains authoritative until the coordinated migration completes end-to-end. Bridge existence alone is not consumption evidence and does not authorize stock mutation.

## 8. Warehouse responsibilities

Склад отвечает за:

- physical spool inventory;
- MaterialLedger generic catalog;
- physical quantity/current stock;
- bounded units;
- purchase/accounting cost;
- ISSUE/RETURN/CORRECTION movements;
- Material Request allocation;
- stock integrity/audit;
- physical-spool ↔ generic-item identity bridge.

Склад не хранит клиентские платежи.

## 9. Costing / Cash responsibilities

Costing читает подтверждённые складом quantities и persisted cost snapshot.

```text
cost_amount = фактическая себестоимость мастерской
charge_amount = сумма, включённая клиенту
```

Cash хранит финансовые события: repair price/charge, payments, corrections/refunds, balance. Cash events не управляют станком и не изменяют склад.

## 10. Archive / corrections

После завершения ремонта material request становится `CLOSED`, но movements не переписываются и доступны через историю client/motor/repair.

После ISSUE correction/return — отдельные append-only events. Финансовые ошибки также исправляются отдельным cash correction/refund event.

## 11. Current implementation status

SOFTWARE GREEN through checkpoint 120:

```text
103 Material Request identity + movement schema
104 CRM backup/export + integrity
105 MaterialLedger serialization fix
106 MaterialLedger adapter + active item lookup
107 Material Request lifecycle + backup/integrity
108+ Material Request warehouse transaction/API and later CRM blocks
117 exact-spool / MaterialLedger migration map
118 append-only spool-material bridge persistence + integrity + backup/export
119 MaterialLedger structured CU/AL + diameter metadata
120 explicit operator-only production bridge API
```

Latest checkpoint-120 evidence on tree `fa651e3e50a25df9489db24b6c71bd853171a9b8`:

```text
CMP Protocol Tests 32944119683 / SUCCESS
ESP32 Build         32944119688 / SUCCESS
```

## 12. Current implementation order

Immediate NEXT:

1. inspect current MaterialLedger usage/adjustment mutation and recovery semantics;
2. inspect current Material Request warehouse coordinator + pending marker;
3. choose one authoritative crash-safe transaction boundary for `RUN_WIRE` ISSUE;
4. couple exact Material Request movement evidence to physical/material stock changes without a split-commit crash window;
5. preserve `material_request_id + source_session_id + source_run_id + physical spool provenance`;
6. integrate costing/finalization/backup/integrity/reports/Web/tests;
7. retire old exact-spool production checks only after the coordinated replacement is fully GREEN;
8. perform final software + hardware acceptance after contracts stabilize.

Do not implement naïve `append movement -> decrement stock` or `decrement stock -> append movement` sequencing without durable pending/recovery evidence.

## 13. Safety / integrity invariants

Не менять:

- `RUN_COMPLETED` never auto-deducts material;
- physical stock movement requires explicit operator action;
- bridge creation itself never mutates stock;
- physical stock mutation and immutable request evidence must recover atomically/fail-closed;
- run-linked wire ISSUE сохраняет exact `source_session_id + source_run_id`;
- current exact spool provenance remains mandatory until coordinated migration completes;
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
67_NEXT_CHAT_HANDOFF_2026-08-22.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
00_READ_FIRST.md
```

Каждый крупный persistence/API/UI блок получает numbered checkpoint с exact commits + CI evidence.
