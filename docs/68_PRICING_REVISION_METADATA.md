# Метаданные редакций цены ремонта

Калькуляция ремонта хранит изменения цены в добавляемом журнале:

`/data/repairs/pricing.ndjson`

Каждое сохранение создаёт новую запись. При загрузке ремонта применяется последняя подходящая запись, а предыдущие остаются в журнале.

## API

`GET /api/repairs/costing?repair_id=<ID>` дополнительно возвращает:

```json
{
  "pricing_revision_count": 3,
  "pricing_updated_at": "2026-08-06T07:00:00.000Z",
  "pricing_revision_source": "APPEND_ONLY_LOG"
}
```

Поля:

- `pricing_revision_count` — количество найденных записей цены для ремонта;
- `pricing_updated_at` — timestamp последней применённой записи или `null`, если цена ещё не сохранялась;
- `pricing_revision_source` — подтверждает, что метаданные получены из append-only журнала ESP32.

Счётчик ограничен значением `65535`, чтобы не допускать переполнения в структуре ESP32.

## Интерфейс

Мобильная и настольная калькуляции показывают:

- количество редакций цены;
- время последнего сохранения;
- подтверждение источника данных.

После успешного сохранения калькуляция повторно загружается, поэтому количество редакций и timestamp обновляются сразу.

## Изменённые файлы

- `firmware/esp32/src/CM_RepairCosting.h`;
- `firmware/esp32/src/CM_RepairCosting.cpp`;
- `firmware/esp32/src/CM_RepairCostingWeb.cpp`;
- `firmware/esp32/web/mobile/costing.html`;
- `firmware/esp32/web/desktop/costing.html`.
