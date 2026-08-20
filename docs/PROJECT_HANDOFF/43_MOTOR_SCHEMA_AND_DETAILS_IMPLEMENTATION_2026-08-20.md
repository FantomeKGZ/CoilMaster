# CoilMaster — Motor schema and details implementation

Дата: **2026-08-20**  
Ветка: `cmp-protocol-v1`

## Статус

Новая модель карточки двигателя и основной UI-блок реализованы на уровне репозитория. Полная PlatformIO/CI/hardware-проверка текущего HEAD ещё не подтверждена.

## Реализованная модель двигателя

Новые/предпочтительные поля:

```text
manufacturer
model
phase_count
slot_count
coil_program
repeat_target
```

Совместимость:

- persisted legacy field `phases` сохранён;
- HTTP API принимает предпочтительный `phase_count` и legacy `phases`;
- если одновременно переданы оба и значения различаются — запрос отклоняется;
- `repeat_target` хранится как `uint16_t`, допустимо `1..65535`;
- новые карточки получают `repeat_target=1` по умолчанию;
- старые записи без `slot_count`, phase или repeat metadata не переписываются и показываются как `не указано`;
- legacy `name` остаётся для backward compatibility, но UI использует `manufacturer + model` как главный заголовок; если оператор не задаёт отдельное обозначение, UI формирует internal legacy `name` из производителя/модели.

## Семантика программы и повторов

```text
38/38 × 6
```

означает один winding JOB:

```text
program = [38, 38]
repeat_target = 6
```

Каждый полный повтор требует отдельного physical START. Сегменты `38/38` принадлежат одному RUN и не создают отдельные run_id.

## Каталог двигателей

Desktop/mobile `motors.html` теперь:

- создаёт карточку с manufacturer/model/phase_count/slot_count/program/repeat_target;
- показывает фазы, пазы и повторы отдельно;
- показывает `не указано` для отсутствующих legacy metadata;
- ищет по manufacturer/model/name/tags/program и числовым полям phases/slots/repeat_target/poles;
- имеет раздельные действия `Подробнее` и `Выбрать для ремонта`.

Desktop quick-add на странице ремонтов приведён к той же схеме. Mobile repair flow уже использует общий motor catalog.

## Motor detail pages

Добавлены:

```text
firmware/esp32/web/desktop/motor-details.html
firmware/esp32/web/mobile/motor-details.html
```

Карточка показывает:

- manufacturer/model/legacy designation;
- phases, slots, poles, connection;
- power, voltage, current, speed, frequency;
- winding program and repeat_target;
- coil pitch, turns, wire diameter, parallel strands, material/type;
- stator bore/core length;
- provenance/source/confidence/calculated-fields marker;
- comment;
- bounded repair history for the exact motor.

## Repair history by exact motor

Registry page reader расширен exact `motor_id` filter. Старый client-only paging API сохранён через backward-compatible overload.

Добавлен endpoint:

```text
GET /api/motors/repairs?motor_id=<id>&cursor=<optional>&limit=<optional>&status=<OPEN|CLOSED optional>
```

Свойства:

- проверяет существование exact motor_id;
- не заставляет браузер сканировать весь repair journal;
- limit ограничен `RepairRegistry::MaxListPageSize`;
- возвращает `has_more` и `next_cursor`;
- detail page показывает OPEN/CLOSED, received/closed timestamps и ссылки на winding history/costing.

## Regression contracts

Добавлены/расширены:

```text
Tests/Web/check_motor_schema_ui.js
Tests/Web/check_motor_details_ui.js
.github/workflows/cmp-protocol-tests.yml
```

Motor details audit проверяет:

- desktop/mobile detail pages;
- exact `/api/motors/by-id`;
- exact `/api/motors/repairs`;
- bounded paging;
- backend exact `lineMotorId != motorId` filter;
- backward-compatible repair-page overload;
- catalog detail links;
- physical START wording.

## Ключевые commits блока

```text
8be2c4e8...  NewMotor repeatTarget
 e9fd76b9... persist repeat_target
60aa0ee7...  phase_count + repeat_target API
 e60b5576... desktop motor schema UI
545d748f...  mobile motor schema UI
6905971a...  desktop repair quick-add schema
79a0917d...  numeric motor search
8586becc...  desktop motor details page
fcb29ac3...  mobile motor details page
3edd5e55...  desktop catalog details link
 e80b988e... mobile catalog details link
683dc3b9...  registry exact motor repair filter
 d18be78e... backward-compatible repair paging overload
79b860bb...  bounded motor repair history endpoint
348dfe0b...  desktop repair history in motor detail
8e5dc98a...  mobile repair history in motor detail
7fb7b120...  motor details regression audit
86dcef46...  workflow integration
```

## Verification status

Подтверждено по текущему коду/контрактам:

- schema/UI wiring;
- exact motor filtering implementation;
- bounded endpoint semantics;
- safety wording and no physical-control additions.

Пока **не подтверждено** для текущего HEAD:

```text
pio run -e uno
pio run -e esp32
CMP Protocol Tests workflow result
hardware ESP32/Arduino E2E
```

Не переносить старый release-green status на этот новый блок.

## Следующий repo-level приоритет

После проверки compile-safety этого блока перейти к redesign Arduino winding archive:

```text
compact rows/table
multi-select checkboxes
program
repeat_target / planned repeats
historical actual run count
status badges
bulk link/create/combine motor actions
```

Связывание archive records с motor records не должно менять immutable physical RUN facts.
