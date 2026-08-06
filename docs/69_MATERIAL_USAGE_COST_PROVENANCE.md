# Источник стоимости списания дополнительных материалов

Успешный ответ `POST /api/materials/usage` теперь явно сообщает, откуда взята стоимость операции и какое правило округления применено.

Пример:

```json
{
  "confirmed": true,
  "usage_id": 17,
  "repair_id": 28,
  "material_id": 4,
  "quantity_milli": 1250,
  "remaining_quantity_milli": 8750,
  "unit_price_minor": 2400,
  "line_cost_minor": 3000,
  "currency": "KGS",
  "repair_reference_validated": true,
  "line_cost_source": "PERSISTED_USAGE_SNAPSHOT",
  "value_rounding": "TRUNCATE_MINOR_UNIT"
}
```

## Значение полей

- `material_id` и `quantity_milli` повторяют подтверждённые параметры операции.
- `unit_price_minor` — цена единицы материала, зафиксированная на момент списания.
- `line_cost_minor` — стоимость, записанная в журнал использования и затем применяемая калькуляцией ремонта.
- `line_cost_source=PERSISTED_USAGE_SNAPSHOT` означает, что стоимость является снимком операции и не пересчитывается по текущей цене материала.
- `value_rounding=TRUNCATE_MINOR_UNIT` документирует действующее правило расчёта: дробная часть минимальной денежной единицы отбрасывается.
- `repair_reference_validated=true` подтверждает проверку существования ремонта до изменения складского остатка.

## Совместимость

Существующие поля ответа сохранены. Новые поля добавлены без изменения формата запроса и без изменения уже записанных операций.

## Изменённый файл

- `firmware/esp32/src/CM_MaterialLedgerWeb.cpp`.
