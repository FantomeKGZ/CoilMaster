# Данные, API, склад, калькуляция и веб-интерфейс

## Общая модель хранения

ESP32 использует microSD и NDJSON-файлы как основное локальное хранилище.

При работе с данными соблюдать правила:

- старая операция не должна менять стоимость после изменения текущей цены;
- подтверждённые движения должны хранить снимок цены и материала;
- незавершённые транзакции должны восстанавливаться после перезапуска;
- неизвестные или старые данные не должны молча превращаться в медь;
- серверные итоги имеют приоритет над повторным расчётом в браузере;
- интерфейс обязан показывать предупреждение при нарушении контрольных сумм.

## Реестр мастерской

Основные сущности:

- клиент;
- двигатель;
- ремонт.

Основные API:

```text
GET/POST /api/clients
GET/POST /api/motors
GET /api/motors/similar
GET/POST /api/repairs
```

Основное хранилище находится под:

```text
/data/workshop/
```

Известный файл ремонтов:

```text
/data/workshop/repairs.ndjson
```

Двигатель содержит, среди прочего:

- название;
- модель;
- производителя;
- теги;
- программу катушек;
- комментарий.

## Поиск похожих двигателей

Текущая логика не является точным сравнением полной обмотки.

Сначала сравнивается нормализованная программа катушек. Затем оцениваются признаки идентичности двигателя.

Категории результата:

```text
LIKELY_SAME_MOTOR
POSSIBLE_ANALOGUE
SAME_PROGRAM_ONLY
```

Правила оценки:

- два и более совпавших признака идентичности — `LIKELY_SAME_MOTOR`;
- один признак — `POSSIBLE_ANALOGUE`;
- только одинаковая программа — `SAME_PROGRAM_ONLY`.

Создание записи не блокируется автоматически:

```json
{"creation_blocked": false}
```

Нельзя описывать эту функцию как точный детектор одинаковой обмотки: полные данные о проводниках пока отсутствуют.

## Выбор двигателя для ремонта

Мобильная и настольная страницы ремонта поддерживают:

```text
?motor_id=<ID>
```

Также используется локальное сохранение выбранного двигателя:

```text
cm-selected-motor
```

Выбранный идентификатор должен проверяться по текущему каталогу.

## Калькулятор

Калькулятор использует сохранённые серверные настройки.

Ключевые правила:

- URL-параметры не должны подменять авторитетные настройки;
- источник настроек обозначается как `PERSISTED`;
- при отсутствии конфигурации возвращается `409` с ошибкой `calculator_not_configured`;
- доступные диаметры берутся из фактического склада выбранного материала;
- каталог провода разделён по CU и AL;
- старые записи с неизвестным материалом исключаются из расчётного каталога.

При отсутствии подходящего провода для материала используется ошибка:

```text
wire_catalogue_empty_for_material
```

## Склад провода

Канонические материалы:

```text
CU
AL
```

Допустимые входные псевдонимы нормализуются:

```text
CU, COPPER, МЕДЬ
AL, ALUMINIUM, ALUMINUM, АЛЮМИНИЙ
```

Старые записи без `wire_type` классифицируются как:

```text
UNKNOWN
```

Нельзя автоматически присваивать им CU.

Фильтры списка катушек:

```text
ALL
CU
AL
UNKNOWN
```

Дополнительно поддерживаются фильтрация по активности и диаметру.

## Однократное назначение материала старой катушке

Для активной старой катушки без `wire_type` реализовано однократное назначение CU или AL.

После назначения повторное изменение этим механизмом запрещено.

Это миграционная операция, а не обычное редактирование материала катушки.

## Списание провода на ремонт

Подтверждённое списание сохраняет снимок материала и цены.

В ответах и истории используются:

- масса до списания;
- масса после списания;
- израсходованная масса;
- цена за килограмм в минимальных денежных единицах;
- стоимость расхода;
- валюта;
- материал CU/AL/UNKNOWN;
- ремонт;
- комментарий;
- timestamp.

Формула стоимости:

```text
ROUND(consumed_grams * price_per_kg_minor / 1000)
```

Фактическая реализация округляет к ближайшей минимальной денежной единице с помощью 64-битного промежуточного значения.

GET истории списаний ремонта возвращает серверные итоги:

- `total_consumed_g`;
- `total_consumed_value_minor`;
- `material_totals` для CU/AL/UNKNOWN;
- число строк по материалам;
- массу по материалам;
- стоимость по материалам.

Контрольные признаки:

```text
material_totals_match_total
material_count_match_count
material_values_match_total
```

Интерфейс использует серверные итоги только когда источник обозначен как `SERVER` и контрольные признаки истинны. Иначе показывается предупреждение и безопасный fallback по полученным строкам.

## Калькуляция ремонта

Стоимость провода разделяется по:

```text
CU
AL
UNKNOWN
```

API калькуляции содержит контрольные признаки:

```text
wire_material_costs_match_wire_cost
wire_material_counts_match_wire_count
```

Историческая стоимость берётся из сохранённых операций, а не пересчитывается по сегодняшней цене катушки.

## Дополнительные материалы

Основные API:

```text
GET  /api/materials
POST /api/materials
POST /api/materials/adjust
GET  /api/materials/adjustments
GET  /api/materials/usage
POST /api/materials/usage
```

Поддерживаемые единицы:

```text
PIECE
GRAM
MILLILITRE
METRE
SQUARE_METRE
```

Основные файлы:

```text
/data/materials/materials.ndjson
/data/materials/usage.ndjson
/data/materials/usage.pending
/data/materials/adjustments.ndjson
```

### Защита списания дополнительного материала

Перед списанием проверяется:

- существование ремонта;
- существование и активность материала;
- валюта материала;
- достаточный остаток;
- корректность количества;
- возможность завершить транзакцию.

Политика валюты:

```text
KGS_ONLY
```

Неизвестный ремонт:

```json
{"error":"repair_not_found"}
```

Материал не найден:

```json
{"error":"material_not_found"}
```

Неподдерживаемая валюта старой записи:

```text
unsupported_material_currency
```

В этих случаях склад не изменяется и pending-транзакция не создаётся.

### Стоимость дополнительного материала

Подтверждённая операция хранит:

- `unit_price_minor`;
- `line_cost_minor`;
- валюту;
- количество;
- остаток;
- ссылку на ремонт.

Формула:

```text
ROUND(quantity_milli * unit_price_minor / 1000)
```

Политика исторической стоимости:

```text
USE_PERSISTED_LINE_COST
```

Источник:

```text
PERSISTED_USAGE_SNAPSHOT
```

API дополнительно сообщает проверки:

```text
line_cost_matches_formula
currency_matches_policy
material_currency_prevalidated
repair_reference_validated
```

Проверки ремонта и валюты выполняются не только веб-обработчиком, но и внутри `MaterialLedger::confirmUsage()`.

## Веб-интерфейс

Есть отдельные мобильные и настольные версии страниц:

```text
firmware/esp32/web/mobile/
firmware/esp32/web/desktop/
```

Ключевые страницы, над которыми уже велась работа:

- `motors.html` — каталог двигателей и похожие записи;
- страницы выбора ремонта и двигателя;
- `writeoff.html` — списание провода и история;
- страницы калькуляции стоимости ремонта.

UI должен:

- не пересчитывать историческую стоимость по новой цене;
- показывать материал катушки;
- блокировать списание UNKNOWN до назначения материала;
- отображать серверные контрольные признаки;
- предупреждать при расхождении итогов;
- одинаково работать в mobile и desktop версиях.

## Важные ограничения

- Один ремонт не должен получать расход от несуществующей складской позиции.
- Списание не должно появляться без существующей карточки ремонта.
- UNKNOWN должен оставаться видимым, но не использоваться как медь по умолчанию.
- Смешанные валюты нельзя молча суммировать. Сейчас проект фактически придерживается KGS_ONLY для дополнительных материалов; при дальнейшем расширении нужна явная политика для всех денежных подсистем.


## Motor and winding data import — 2026-08-12

`POST /api/motors` remains a one-record append endpoint and now accepts optional structured motor, winding, geometry, and provenance fields documented in `docs/MOTOR_IMPORT_FORMAT.md`.

Browser import pages:

- `/desktop/motor-import.html`
- `/mobile/motor-import.html`

They accept a JSON array of 1–50 records. Preview validates every object and queries `GET /api/motors/similar`; it performs no writes. Exact identity matches are excluded by default. Import requires explicit selection and confirmation, then submits records sequentially so one failed record does not silently hide the remaining result.

Every sourced import requires `source_type + source_title + confidence`. Calculated values are explicitly marked by `calculated_fields`. Existing motor NDJSON records remain readable because all new fields are optional.


## Motor import hardening and verification — 2026-08-15

The 1–50 row import remains review-first and sequential, but validation is now
strict:

- only documented field names are accepted;
- text fields are trimmed and bounded;
- `source_retrieved_at` is a real `YYYY-MM-DD` date in 2000–2199;
- optional `source_url` is HTTP(S);
- `CALCULATED` classification and `calculated_fields` must agree;
- duplicate identities inside the same package are rejected before lookup/write;
- successful rows cannot be resubmitted from the same preview;
- failed rows remain available for an explicit retry.

`POST /api/motors` independently repeats the provenance and length checks before
the append. Existing unsourced manual motor creation remains supported because a
request with no source metadata is still valid.

The executable web audit covers the documented valid example plus unknown-field,
invalid-date, calculated-provenance and package-duplicate cases for desktop and
mobile. Verified checkpoint:

```text
684e848c235b5f37607e9ca814e8bc11647c1b5d
CMP Protocol Tests + motor-import web audit: SUCCESS
ESP32 Build: SUCCESS
RAM: 15.7% (51408 / 327680 bytes)
Flash: 41.8% (1314657 / 3145728 bytes)
```
