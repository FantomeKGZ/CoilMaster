# Серверные итоги расхода по материалам

`GET /api/warehouse/write-offs?repair_id=<ID>` возвращает общий подтверждённый расход и серверную разбивку по материалам.

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
  "material_totals_source": "SERVER",
  "material_totals_match_total": true,
  "material_count_match_count": true
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

## Проверки согласованности

`material_totals_match_total` имеет значение `true`, когда:

```text
CU.consumed_g + AL.consumed_g + UNKNOWN.consumed_g = total_consumed_g
```

`material_count_match_count` имеет значение `true`, когда:

```text
CU.count + AL.count + UNKNOWN.count = count
```

Обе проверки вычисляются ESP32 после чтения журнала. Интерфейс может использовать их для обнаружения внутренней несогласованности ответа и не должен подменять серверные значения собственными итогами.

## Совместимость

Существующие поля `items`, `count` и `total_consumed_g` сохранены. `material_totals` и проверки согласованности добавлены без изменения структуры элементов истории.

Мобильный и настольный интерфейсы предыдущей версии продолжают работать, поскольку могут рассчитать разбивку из `items`. Следующий этап — использовать серверные итоги как основной источник, оставив клиентский расчёт только для совместимости со старой прошивкой.

## Изменённые файлы

- `firmware/esp32/src/CM_WarehouseStore.h`;
- `firmware/esp32/src/CM_WarehouseWriteOffHistory.cpp`;
- `firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp`.
