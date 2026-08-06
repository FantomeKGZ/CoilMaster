# Серверные итоги расхода по материалам

`GET /api/warehouse/write-offs?repair_id=<ID>` теперь возвращает не только общий подтверждённый расход, но и серверную разбивку по материалам.

Пример:

```json
{
  "repair_id": 28,
  "items": [],
  "count": 4,
  "total_consumed_g": 910,
  "material_totals": {
    "CU": {
      "consumed_g": 540,
      "count": 2
    },
    "AL": {
      "consumed_g": 300,
      "count": 1
    },
    "UNKNOWN": {
      "consumed_g": 70,
      "count": 1
    }
  },
  "material_totals_source": "SERVER"
}
```

## Правила расчёта

- учитываются только движения `WRITE_OFF` со статусом `CONFIRMED`;
- учитываются только движения выбранного `repair_id`;
- `wire_type=CU` относится к меди;
- `wire_type=AL` относится к алюминию;
- отсутствующий или неизвестный `wire_type` относится к `UNKNOWN`;
- `UNKNOWN` никогда не преобразуется в медь автоматически;
- `count` внутри каждой группы — количество подтверждённых операций;
- `consumed_g` — сумма поля `mass_g` соответствующей группы.

## Совместимость

Существующие поля `items`, `count` и `total_consumed_g` сохранены. Новое поле `material_totals` добавлено без изменения прежней структуры списка.

Мобильный и настольный интерфейсы предыдущей версии продолжают работать, так как могут рассчитать разбивку из `items`. На следующем этапе они будут переведены на серверные итоги с клиентским расчётом только как резервным вариантом.

## Изменённые файлы

- `firmware/esp32/src/CM_WarehouseStore.h`;
- `firmware/esp32/src/CM_WarehouseWriteOffHistory.cpp`;
- `firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp`.
