# Autonomous archive bounded paging — 2026-08-12

Ветка: `cmp-protocol-v1`

## Цель блока

Убрать рост RAM одного HTTP-ответа вместе со всем `/data/autonomous-windings/events.ndjson` и подготовить Arduino archive к длительной эксплуатации без миграции в БД и без ротации/удаления исторических записей.

## Что изменено

### 1. Bounded API page

`GET /api/autonomous-windings` теперь использует paged reader.

Параметры:

```text
limit   default 20, max 32
cursor  opaque byte offset, default 0
program optional winding program filter
tolerance_percent 0..50
```

Ответ содержит:

```text
items
count
limit
cursor
has_more
next_cursor
max_page_size
```

`next_cursor` выдаётся только firmware и указывает на границу следующей логической задачи.

### 2. Cursor не может разрезать run

Archive остаётся append-only NDJSON. Cursor принимается только на границе строки.

Одна логическая задача формируется так:

```text
RUN_STARTED + matching RUN_COMPLETED -> one COMPLETED API item
RUN_STARTED without completion       -> STARTED_NOT_COMPLETED
RUN_COMPLETED start_observed=0        -> recovered COMPLETED item
```

Cursor никогда не возвращается между нормальной парой `RUN_STARTED/RUN_COMPLETED`.

Если входной cursor попал на `RUN_COMPLETED` с `start_observed=1` без прочитанного matching START, запрос fail-closed отклоняется.

### 3. Bounded ESP32 response RAM

HTTP handler больше не формирует JSON для всего архива. Максимум одной страницы — 32 logical tasks.

Reserve привязан к page limit, а не к размеру истории.

### 4. Assignment lookup больше не N full scans per page

Paged reader сначала собирает bounded page, затем одним проходом `assignments.ndjson` подставляет последнюю assignment для всех completed tasks этой страницы.

Это сохраняет existing latest-assignment semantics и убирает старый `latestAssignment()` full scan для каждого отдельного элемента page.

### 5. Mobile/Desktop UI

Обе страницы переведены на cursor pagination:

```text
firmware/esp32/web/mobile/arduino-windings.html
firmware/esp32/web/desktop/arduino-windings.html
```

Обычный список:

```text
20 tasks
→ Показать ещё
→ next_cursor
```

Раздел «Скомплектованные двигатели» больше не запускает второй полный archive request автоматически при каждом поиске.

Полная выборка для комплектаций выполняется только после явной кнопки:

```text
Загрузить / обновить комплектации
```

и также читается bounded pages по 32 записи. В browser memory сохраняются только assigned completed tasks для этого представления.

## Формат хранения не изменён

Остаются authoritative файлы:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

Нет:

- DB migration;
- destructive compaction;
- automatic rotation;
- удаления historical STARTED_NOT_COMPLETED;
- изменения safety semantics.

## Commits

```text
0ed6402d6665513715626ccb386a06ca3db1b4c8  Add bounded autonomous archive page API
231b3e13cc60333f29abdf858b68891443455f3b  Implement bounded autonomous archive paging
2468bb00fb1616883cb3623c6d8964f63553aa18  Bound autonomous archive HTTP responses
e2fd603f6ebe76572c3891a6a5e7e76c28db9099  Page mobile autonomous winding archive
fee1b2349a4ab87d3b0ddf5922da33195729aa7f  Page desktop autonomous winding archive
c812a9c2180e265ee296c6c9fd282719431e3058  Make autonomous page reader C++11 safe
```

## Verification status

Изменения добавлены после последнего подтверждённого ESP32 build.

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

Нужен clean ESP32 build.

После прошивки для нового UI необходимо заменить содержимое microSD `/web` актуальным `firmware/esp32/web`.

## Следующий performance шаг

После build/runtime проверки paging — перевести boot-time `AutonomousWindingArchive::begin()` со старых repeated validators на уже реализованный authoritative bounded-complexity `validateStorage()` и затем снять populated-dataset timings через backup manifest.

Rotation threshold не вводить до реальных metrics.
