# API двустороннего калькулятора проводника

## Назначение

Калькулятор использует весь каталог диаметров, которые когда-либо вводились в базу бухт. Текущие активные остатки суммируются по диаметру. Если известный диаметр сейчас отсутствует на складе, он остаётся участником расчёта со статусом `PURCHASE_REQUIRED`.

## Маршрут

`GET /api/calculator/conductor`

## Обязательные параметры

- `source_material` — `AL` или `CU`;
- `target_material` — `CU` или `AL`;
- `source_diameter_hundredths_mm` — диаметр исходной жилы в сотых долях миллиметра;
- `source_parallel_strands` — количество параллельных исходных жил.

## Необязательные параметры

- `al_to_cu_permille` — коэффициент алюминий → медь;
- `cu_to_al_permille` — коэффициент медь → алюминий;
- `allowed_deviation_permille` — допустимое отклонение;
- `max_target_strands` — максимальное число параллельных целевых жил, от 1 до 8.

Пока постоянные коэффициенты мастерской не утверждены, API принимает их как параметры. Позднее они будут читаться из настроек.

## Пример

`/api/calculator/conductor?source_material=AL&target_material=CU&source_diameter_hundredths_mm=80&source_parallel_strands=3&al_to_cu_permille=700&allowed_deviation_permille=80&max_target_strands=4`

## Ответ

API возвращает до трёх лучших рекомендаций:

```json
{
  "source_material": "AL",
  "target_material": "CU",
  "source_diameter_hundredths_mm": 80,
  "source_parallel_strands": 3,
  "required_target_area_um2": 1055575,
  "catalogue_diameter_count": 18,
  "recommendation_count": 3,
  "recommendations": [
    {
      "rank": 1,
      "diameter_hundredths_mm": 82,
      "parallel_strands": 2,
      "target_area_um2": 1056265,
      "deviation_permille": 0,
      "available_g": 4700,
      "availability": "IN_STOCK"
    },
    {
      "rank": 2,
      "diameter_hundredths_mm": 83,
      "parallel_strands": 2,
      "target_area_um2": 1082100,
      "deviation_permille": 25,
      "available_g": 0,
      "availability": "PURCHASE_REQUIRED"
    }
  ]
}
```

## Каталог

Источником служит `/data/warehouse/spools.ndjson`:

- каждый уникальный введённый диаметр входит в каталог;
- веса активных бухт складываются;
- неактивные и пустые позиции сохраняют известность диаметра;
- варианты в наличии получают приоритет;
- отсутствующие варианты используются как совет для закупки.

## Ограничение текущего этапа

Текущая версия подбирает один диаметр в одной или нескольких параллельных жилах. Смешанные наборы разных диаметров будут отдельным расширением после проверки базового расчёта на реальных двигателях.
