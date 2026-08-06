# Проверка истории цены против текущей калькуляции

`GET /api/repairs/pricing-history?repair_id=<ID>` теперь возвращает текущий активный снимок цены и серверные проверки его соответствия последней записи append-only журнала.

Пример:

```json
{
  "current_pricing": {
    "labour_cost_minor": 120000,
    "client_price_minor": 280000,
    "currency": "KGS",
    "updated_at": "2026-08-06T07:00:00.000Z"
  },
  "history_count_matches_current": true,
  "history_latest_values_match_current": true,
  "history_latest_timestamp_matches_current": true,
  "history_matches_current_pricing": true,
  "current_pricing_source": "LATEST_APPEND_ONLY_REVISION",
  "source": "APPEND_ONLY_LOG"
}
```

## Проверки

- `history_count_matches_current` — количество записей истории совпадает с `pricing_revision_count`, полученным текущей калькуляцией.
- `history_latest_values_match_current` — стоимость работы, цена клиенту и валюта последней записи совпадают с активными значениями.
- `history_latest_timestamp_matches_current` — timestamp последней записи совпадает с `pricing_updated_at`.
- `history_matches_current_pricing` — все три проверки выполнены одновременно.

Для ремонта без сохранённой цены корректным считается пустой журнал, нулевые активные значения и отсутствие `pricing_updated_at`.

## Реализация

`PricingRevisionSnapshot` заполняется непосредственно во время чтения истории. Поэтому сравнение выполняется с точной последней валидной записью, а не только с фактом наличия timestamp.

Изменённые файлы:

- `firmware/esp32/src/CM_RepairCosting.h`;
- `firmware/esp32/src/CM_RepairPricingHistory.cpp`;
- `firmware/esp32/src/CM_RepairCostingWeb.cpp`.
