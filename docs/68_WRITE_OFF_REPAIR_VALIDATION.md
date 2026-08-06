# Проверка ремонта при списании провода

Списание провода теперь разрешено только для ремонта, который существует в реестре:

```text
/data/workshop/repairs.ndjson
```

## API

Для неизвестного `repair_id` оба запроса возвращают:

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

- `POST /api/warehouse/write-offs`;
- `GET /api/warehouse/write-offs?repair_id=<ID>`.

## Защита на уровне хранилища

`WarehouseStore::confirmSpoolWriteOff()` повторно проверяет существование ремонта до создания записи `PENDING` и до изменения веса бухты. Поэтому обход HTTP-проверки не позволяет записать расход на несуществующий ремонт.

## Ответ успешного списания

Успешный `POST /api/warehouse/write-offs` теперь гарантированно возвращает стоимость зафиксированной операции:

```json
{
  "confirmed": true,
  "consumed_g": 320,
  "price_per_kg_minor": 85000,
  "consumed_value_minor": 27200,
  "currency": "KGS",
  "value_rounding": "NEAREST_MINOR_UNIT"
}
```

Формула:

```text
(mass_g × price_per_kg_minor + 500) / 1000
```

Расчёт выполняется через `uint64_t`.

## Изменённые файлы

- `firmware/esp32/src/CM_WarehouseStore.h`;
- `firmware/esp32/src/CM_WarehouseRepairValidation.cpp`;
- `firmware/esp32/src/CM_WarehouseWriteOff.cpp`;
- `firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp`.
