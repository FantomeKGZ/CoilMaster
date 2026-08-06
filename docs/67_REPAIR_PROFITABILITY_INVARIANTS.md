# Проверки согласованности прибыльности ремонта

Ответ `GET /api/repairs/costing?repair_id=<ID>` дополнен серверными признаками доверия к расчёту цены и себестоимости.

```json
{
  "cost_components_match_total": true,
  "pricing_status_source": "SERVER",
  "pricing_delta_matches_price": true
}
```

## Проверка себестоимости

`cost_components_match_total` подтверждает равенство:

```text
wire_cost_minor + material_cost_minor + labour_cost_minor = total_cost_minor
```

## Проверка цены, маржи и убытка

`pricing_delta_matches_price` проверяет один из вариантов:

- цена не задана: `client_price_minor=0`, маржа и убыток равны нулю;
- прибыль: `client_price_minor = total_cost_minor + margin_minor`, убыток равен нулю;
- безубыточность: цена равна себестоимости, маржа и убыток равны нулю;
- убыток: `total_cost_minor = client_price_minor + loss_minor`, маржа равна нулю.

Поле `pricing_status_source` имеет значение `SERVER`, когда статус `NOT_SET`, `PROFIT`, `BREAK_EVEN` или `LOSS` рассчитан ESP32.

## Интерфейс

Мобильная и настольная калькуляции показывают статус прибыльности только как доверенный, если одновременно выполняются три условия:

- `pricing_status_source=SERVER`;
- `pricing_delta_matches_price=true`;
- `cost_components_match_total=true`.

При нарушении хотя бы одного условия интерфейс выводит предупреждение и не представляет статус как подтверждённый сервером.

## Изменённые файлы

- `firmware/esp32/src/CM_RepairCostingWeb.cpp`;
- `firmware/esp32/web/mobile/costing.html`;
- `firmware/esp32/web/desktop/costing.html`.
