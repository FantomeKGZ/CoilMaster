# Предварительная проверка валюты дополнительного материала

Перед подтверждением расхода через:

```text
POST /api/materials/usage
```

ESP32 теперь загружает валюту выбранного активного материала из складского реестра и проверяет её до изменения остатка.

## Политика

Поддерживается только:

```text
KGS
```

Это защищает калькуляцию ремонта от смешивания денежных сумм в разных валютах.

## Неизвестный материал

Если `material_id` отсутствует или материал не активен, сервер возвращает:

```json
{
  "error": "material_not_found"
}
```

HTTP-статус: `404`.

## Неподдерживаемая валюта старой записи

Для старого материала, сохранённого не в KGS, сервер возвращает:

```json
{
  "error": "unsupported_material_currency",
  "material_id": 12,
  "material_currency": "USD",
  "supported_currency": "KGS",
  "currency_policy": "KGS_ONLY",
  "write_performed": false
}
```

HTTP-статус: `409`.

При таком ответе:

- остаток материала не изменяется;
- запись расхода не создаётся;
- pending-транзакция не запускается;
- стоимость ремонта не изменяется.

## Успешный ответ

Успешное списание дополнительно содержит:

```json
{
  "material_currency_prevalidated": true,
  "currency_matches_policy": true,
  "currency_policy": "KGS_ONLY"
}
```

## Реализация

Добавлен метод:

```cpp
bool MaterialLedger::loadActiveMaterialCurrency(
    uint32_t materialId,
    String& currency) const;
```

Изменённые файлы:

- `firmware/esp32/src/CM_MaterialLedger.h`;
- `firmware/esp32/src/CM_MaterialLedgerCurrency.cpp`;
- `firmware/esp32/src/CM_MaterialLedgerWeb.cpp`.
