# Защита от перезаписи устаревшей цены ремонта

Сохранение калькуляции использует номер редакции, загруженный оператором перед редактированием.

## Запрос

`POST /api/repairs/costing`

Обязательные поля:

- `repair_id`;
- `expected_revision`;
- `labour_cost_minor`;
- `client_price_minor`;
- `currency`;
- `timestamp`.

`expected_revision` должен совпадать с текущим `pricing_revision_count` на ESP32.

## Успешное сохранение

```json
{
  "saved": true,
  "previous_revision": 4,
  "new_revision": 5,
  "revision_source": "APPEND_ONLY_LOG"
}
```

## Конфликт редакций

Если цена была изменена после загрузки страницы, ESP32 не добавляет новую запись и возвращает HTTP 409:

```json
{
  "error": "pricing_revision_conflict",
  "expected_revision": 4,
  "current_revision": 5,
  "reload_required": true
}
```

Это предотвращает незаметную перезапись более новой цены данными из другой вкладки или другого устройства.

## Интерфейс

Мобильная и настольная страницы:

- запоминают номер загруженной редакции;
- отправляют его как `expected_revision`;
- блокируют кнопку на время запроса;
- при конфликте обновляют данные с ESP32;
- требуют от оператора повторно проверить цену перед новым сохранением.

## Изменённые файлы

- `firmware/esp32/src/CM_RepairCostingWeb.cpp`;
- `firmware/esp32/web/mobile/costing.html`;
- `firmware/esp32/web/desktop/costing.html`.
