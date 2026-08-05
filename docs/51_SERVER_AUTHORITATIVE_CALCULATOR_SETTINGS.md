# Серверные настройки калькулятора проводника

## Изменение

`GET /api/calculator/conductor` больше не принимает коэффициенты расчёта из запроса пользователя.

Калькулятор использует только настройки, сохранённые на ESP32 через:

- `GET /api/calculator/settings`
- `POST /api/calculator/settings`

Файл хранения:

`/data/settings/conductor.json`

## Обязательные параметры расчёта

Клиент передаёт только исходные данные:

- `source_material`
- `target_material`
- `source_diameter_hundredths_mm`
- `source_parallel_strands`

Параметры `al_to_cu_permille`, `cu_to_al_permille`, `allowed_deviation_permille`, `max_target_strands` и `allow_mixed_diameters` из URL больше не влияют на результат.

## Ошибка конфигурации

Если настройки ещё не сохранены, API возвращает:

```json
{"error":"calculator_not_configured"}
```

HTTP-статус: `409 Conflict`.

Мобильная и настольная страницы показывают ссылку на раздел настроек и не запускают расчёт до сохранения конфигурации.

## Ответ расчёта

Ответ содержит `settings_source: "PERSISTED"` и фактически применённые параметры. Это позволяет проверить, по какой методике выполнен расчёт.
