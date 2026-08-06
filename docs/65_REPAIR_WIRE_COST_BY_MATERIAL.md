# Стоимость провода по материалам в калькуляции ремонта

`GET /api/repairs/costing?repair_id=<ID>` возвращает серверную разбивку подтверждённых списаний провода:

```json
{
  "wire_cost_minor": 41500,
  "wire_materials": {
    "CU": {
      "consumed_g": 320,
      "line_count": 1,
      "cost_minor": 27200
    },
    "AL": {
      "consumed_g": 260,
      "line_count": 1,
      "cost_minor": 14300
    },
    "UNKNOWN": {
      "consumed_g": 0,
      "line_count": 0,
      "cost_minor": 0
    }
  },
  "wire_materials_source": "CONFIRMED_WRITE_OFFS",
  "wire_material_costs_match_wire_cost": true,
  "wire_material_counts_match_wire_count": true
}
```

## Проверки согласованности

- `wire_material_costs_match_wire_cost` подтверждает, что сумма стоимости `CU`, `AL` и `UNKNOWN` совпадает с `wire_cost_minor`.
- `wire_material_counts_match_wire_count` подтверждает, что сумма количества операций по материалам совпадает с `wire_line_count`.
- В стоимость входят только движения `WRITE_OFF` со статусом `CONFIRMED`.
- Стоимость каждой строки рассчитывается по цене, сохранённой в движении на момент списания.
- Отсутствующий или неизвестный `wire_type` относится к `UNKNOWN` и не считается медью.

## Интерфейс

Мобильная и настольная страницы калькуляции показывают для каждого материала:

- подтверждённую массу;
- количество операций;
- стоимость.

При корректных проверках интерфейс сообщает, что источник разбивки — подтверждённые списания ESP32. При расхождении выводится явное предупреждение.

## Изменённые файлы

- `firmware/esp32/src/CM_RepairCostingWeb.cpp`;
- `firmware/esp32/web/mobile/costing.html`;
- `firmware/esp32/web/desktop/costing.html`.
