# Проверка ремонта в калькуляции

Калькуляция и история цены теперь работают только с ремонтом, который существует в реестре:

```text
/data/workshop/repairs.ndjson
```

## API

Для неизвестного `repair_id` возвращается:

```http
HTTP/1.1 404 Not Found
Content-Type: application/json; charset=utf-8
```

```json
{
  "error": "repair_not_found"
}
```

Проверка применяется к:

- `GET /api/repairs/costing?repair_id=<ID>`;
- `POST /api/repairs/costing`;
- `GET /api/repairs/pricing-history?repair_id=<ID>`.

## Защита хранилища

Проверка выполнена не только в HTTP-обработчиках:

- `RepairCosting::load()` отклоняет неизвестный ремонт;
- `RepairCosting::savePricing()` повторно проверяет ремонт перед добавлением ревизии цены;
- данные для несуществующего ремонта не записываются в `pricing.ndjson` даже при обходе веб-маршрута.

Таким образом, калькуляция не создаёт «осиротевшие» записи с номером ремонта, которого нет в реестре.

## Изменённые файлы

- `firmware/esp32/src/CM_RepairCosting.h`;
- `firmware/esp32/src/CM_RepairCostingValidation.cpp`;
- `firmware/esp32/src/CM_RepairCosting.cpp`;
- `firmware/esp32/src/CM_RepairCostingWeb.cpp`.
