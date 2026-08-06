# Стоимость подтверждённых списаний по ремонту

`GET /api/warehouse/write-offs?repair_id=<ID>` теперь возвращает стоимость каждой операции списания и серверные итоги стоимости.

## Поля операции

Каждый элемент `items` содержит:

```json
{
  "consumed_g": 320,
  "price_per_kg_minor": 85000,
  "consumed_value_minor": 27200,
  "currency": "KGS"
}
```

`consumed_value_minor` рассчитывается из массы и цены, сохранённых в движении на момент подтверждения списания.

## Общие итоги

Ответ содержит:

```json
{
  "total_consumed_value_minor": 41500,
  "material_totals": {
    "CU": {
      "consumed_g": 320,
      "count": 1,
      "consumed_value_minor": 27200
    },
    "AL": {
      "consumed_g": 260,
      "count": 1,
      "consumed_value_minor": 14300
    },
    "UNKNOWN": {
      "consumed_g": 0,
      "count": 0,
      "consumed_value_minor": 0
    }
  },
  "material_values_match_total": true,
  "value_rounding": "NEAREST_MINOR_UNIT"
}
```

## Формула

Для одной операции:

```text
consumed_value_minor = round(consumed_g × price_per_kg_minor / 1000)
```

В целочисленном коде ESP32 используется `uint64_t`:

```text
(mass_g × price_per_kg_minor + 500) / 1000
```

Это исключает переполнение 32-битного произведения и округляет результат до ближайшей минимальной денежной единицы вместо систематического округления вниз.

Та же формула теперь применяется в модуле калькуляции ремонта, поэтому стоимость провода в истории списаний и в `/api/repairs/costing` рассчитывается одинаково.

## Совместимость

Существующие поля истории расхода сохранены. Новые поля добавлены без изменения формата ранее доступных данных.

Записи без `wire_type` остаются в группе `UNKNOWN` и не преобразуются в медь автоматически.

## Изменённые файлы

- `firmware/esp32/src/CM_WarehouseStore.h`;
- `firmware/esp32/src/CM_WarehouseWriteOffHistory.cpp`;
- `firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp`;
- `firmware/esp32/src/CM_RepairCosting.cpp`.
