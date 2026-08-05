# Настройки калькулятора проводника

Параметры методики теперь сохраняются на microSD и не должны вводиться при каждом расчёте.

## Файл

`/data/settings/conductor.json`

## API

- `GET /api/calculator/settings` — прочитать текущие настройки;
- `POST /api/calculator/settings` — сохранить настройки.

Поля:

- `aluminium_to_copper_permille`;
- `copper_to_aluminium_permille`;
- `allowed_deviation_permille`;
- `max_target_strands`;
- `allow_mixed_diameters`.

Диапазоны проверки:

- коэффициенты: `100…3000`;
- допуск: `1…500` промилле;
- количество целевых жил: `1…8`.

При отсутствии файла API возвращает безопасные значения структуры `ConversionSettings` и `configured:false`.

Следующий шаг: добавить эти поля в мобильную и ПК-страницы настроек, а страницы калькулятора перевести на автоматическую загрузку сохранённой методики.
