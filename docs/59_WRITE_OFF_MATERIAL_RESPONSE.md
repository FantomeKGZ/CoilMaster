# Материал в ответе подтверждённого списания

## Изменение API

Успешный ответ:

`POST /api/warehouse/write-offs`

теперь содержит материал бухты, с которой выполнено списание.

Для классифицированной бухты:

```json
{
  "confirmed": true,
  "movement_id": 17,
  "spool_id": 4,
  "repair_id": 28,
  "diameter_hundredths_mm": 80,
  "wire_type": "CU",
  "legacy_unknown_material": false,
  "consumed_g": 320,
  "current_weight_g": 7080,
  "price_per_kg_minor": 85000,
  "currency": "KGS"
}
```

Для старой бухты без подтверждённого материала:

```json
{
  "wire_type": null,
  "legacy_unknown_material": true
}
```

Неизвестный материал не преобразуется в `CU` автоматически.

## Backend

`SpoolWriteOffResult` получил поле `wireType`.

После успешной записи `CONFIRMED` метод `confirmSpoolWriteOff` возвращает тот же материал, который был считан из карточки бухты и сохранён в журнале движения.

Это позволяет интерфейсам ремонта и складского учёта показывать оператору, из какого материала был фактически списан провод, без повторного запроса карточки бухты.

## Изменённые файлы

- `firmware/esp32/src/CM_WarehouseStore.h`;
- `firmware/esp32/src/CM_WarehouseWriteOff.cpp`;
- `firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp`.
