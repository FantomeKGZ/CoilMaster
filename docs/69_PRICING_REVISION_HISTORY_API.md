# История редакций цены ремонта

Добавлен запрос:

`GET /api/repairs/pricing-history?repair_id=<ID>`

Он возвращает все сохранённые редакции цены выбранного ремонта из append-only журнала:

`/data/repairs/pricing.ndjson`

Пример ответа:

```json
{
  "repair_id": 28,
  "items": [
    {
      "revision": 1,
      "labour_cost_minor": 120000,
      "client_price_minor": 280000,
      "currency": "KGS",
      "timestamp": "2026-08-06T06:00:00.000Z"
    },
    {
      "revision": 2,
      "labour_cost_minor": 130000,
      "client_price_minor": 300000,
      "currency": "KGS",
      "timestamp": "2026-08-06T07:00:00.000Z"
    }
  ],
  "count": 2,
  "latest_revision": 2,
  "source": "APPEND_ONLY_LOG"
}
```

## Правила

- `repair_id` обязателен и должен быть положительным целым числом;
- редакции возвращаются в порядке записи в журнал;
- поле `revision` формируется как последовательный номер записи выбранного ремонта;
- предыдущие редакции не удаляются и не перезаписываются;
- при отсутствии записей возвращается пустой массив и `count: 0`;
- чтение истории не изменяет калькуляцию;
- последняя редакция остаётся активной для текущей калькуляции.

## Изменённые файлы

- `firmware/esp32/src/CM_RepairCosting.h`;
- `firmware/esp32/src/CM_RepairPricingHistory.cpp`;
- `firmware/esp32/src/CM_RepairCostingWeb.h`;
- `firmware/esp32/src/CM_RepairCostingWeb.cpp`.
