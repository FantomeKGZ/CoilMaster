# Разделение стоимости провода по материалам в калькуляции ремонта

API калькуляции ремонта теперь возвращает стоимость и массу списанного провода отдельно для меди, алюминия и старых движений без подтверждённого материала.

Запрос:

`GET /api/repairs/costing?repair_id=<ID>`

Новая часть ответа:

```json
{
  "wire_cost_minor": 32500,
  "wire_materials": {
    "CU": {
      "consumed_g": 320,
      "line_count": 1,
      "cost_minor": 27200
    },
    "AL": {
      "consumed_g": 100,
      "line_count": 1,
      "cost_minor": 5300
    },
    "UNKNOWN": {
      "consumed_g": 0,
      "line_count": 0,
      "cost_minor": 0
    }
  },
  "wire_materials_source": "CONFIRMED_WRITE_OFFS"
}
```

## Правила расчёта

Для каждой подтверждённой операции списания используется цена, сохранённая в самой операции:

`line_cost_minor = mass_g × price_per_kg_minor / 1000`

Расчёт выполняется с использованием 64-битного целого числа.

- `CU` — подтверждённые медные списания;
- `AL` — подтверждённые алюминиевые списания;
- `UNKNOWN` — старые подтверждённые движения без `wire_type`;
- `wire_cost_minor` сохраняется как общий итог всех трёх групп;
- `UNKNOWN` не считается медью автоматически;
- общая калькуляция ремонта по-прежнему складывает провод, дополнительные материалы и работу.

## Изменённые файлы

- `firmware/esp32/src/CM_RepairCosting.h`;
- `firmware/esp32/src/CM_RepairCosting.cpp`;
- `firmware/esp32/src/CM_RepairCostingWeb.cpp`.
