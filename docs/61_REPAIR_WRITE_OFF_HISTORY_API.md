# История списаний провода по ремонту

Добавлен запрос:

`GET /api/warehouse/write-offs?repair_id=<ID>`

Он возвращает только подтверждённые движения `WRITE_OFF` со статусом `CONFIRMED`, относящиеся к выбранному ремонту.

Пример ответа:

```json
{
  "repair_id": 28,
  "items": [
    {
      "movement_id": 17,
      "spool_id": 4,
      "repair_id": 28,
      "diameter_hundredths_mm": 80,
      "wire_type": "CU",
      "legacy_unknown_material": false,
      "weight_before_g": 7400,
      "weight_after_g": 7080,
      "consumed_g": 320,
      "price_per_kg_minor": 85000,
      "currency": "KGS",
      "timestamp": "2026-08-06T04:45:00.000Z",
      "comment": "Основная обмотка"
    }
  ],
  "count": 1,
  "total_consumed_g": 320
}
```

Для старого движения без сохранённого материала:

```json
{
  "wire_type": null,
  "legacy_unknown_material": true
}
```

## Правила

- `repair_id` обязателен и должен быть положительным целым числом.
- `PENDING` записи не возвращаются.
- Сумма `total_consumed_g` строится только по возвращённым подтверждённым движениям.
- История не изменяет склад и является только чтением журнала.
- Материал `UNKNOWN` не преобразуется в медь автоматически.

## Изменённые файлы

- `firmware/esp32/src/CM_WarehouseStore.h`;
- `firmware/esp32/src/CM_WarehouseWriteOffHistory.cpp`;
- `firmware/esp32/src/CM_WarehouseWeb.h`;
- `firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp`.
