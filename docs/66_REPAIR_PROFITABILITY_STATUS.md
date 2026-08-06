# Статус прибыльности ремонта

`GET /api/repairs/costing?repair_id=<ID>` теперь возвращает отдельные поля для прибыльности ремонта:

```json
{
  "client_price_set": true,
  "margin_minor": 25000,
  "loss_minor": 0,
  "pricing_status": "PROFIT"
}
```

## Возможные статусы

- `NOT_SET` — конечная цена клиенту равна нулю и ещё не задана;
- `PROFIT` — цена клиенту выше полной себестоимости;
- `BREAK_EVEN` — цена клиенту равна полной себестоимости;
- `LOSS` — цена клиенту ниже полной себестоимости.

## Расчёт

- `margin_minor = client_price_minor - total_cost_minor`, только когда цена выше себестоимости;
- `loss_minor = total_cost_minor - client_price_minor`, только когда цена ниже себестоимости;
- одно из полей `margin_minor` или `loss_minor` всегда равно нулю;
- нулевая цена не считается убытком, а обозначается статусом `NOT_SET`.

## Интерфейс

Мобильная и настольная страницы калькуляции показывают:

- маржу;
- убыток;
- явный статус цены.

При статусе `PROFIT` используется положительное уведомление. Для `BREAK_EVEN`, `LOSS` и `NOT_SET` выводится предупреждение.

## Изменённые файлы

- `firmware/esp32/src/CM_RepairCostingWeb.cpp`;
- `firmware/esp32/web/mobile/costing.html`;
- `firmware/esp32/web/desktop/costing.html`.
