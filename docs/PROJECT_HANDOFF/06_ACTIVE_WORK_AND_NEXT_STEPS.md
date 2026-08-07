# Где остановились и что делать дальше

Дата обновления: 2026-08-07
Ветка: `cmp-protocol-v1`

## Подтверждённое состояние

Пользователь подтвердил зелёные проверки для последних этапов:

- `ESP32 Build #357` и `CMP Protocol Tests #610` — строгая сверка runtime-state с immutable snapshot;
- `ESP32 Build #359` и `CMP Protocol Tests #612` — типизированное чтение immutable snapshot.

Последний коммит интеграции отображаемой программы:

```text
232151f58e02f58b934663b010bb5d9b4c9d3311
Restore immutable job program for display
```

Его CI ещё нужно подтвердить отдельно перед следующим изменением, влияющим на прошивку.

## Что реализовано

Полный путь создания задания на ESP32 теперь выглядит так:

```text
HTTP/UI validation
→ persistent job_id/session_id allocation
→ immutable snapshot persistence and verification
→ runtime-state creation
→ DELIVERING persistence
→ queueJob()
→ JOB_ACK persistence
→ RUN_STARTED/RUN_COMPLETED journal and state persistence
```

Реализованы:

- устойчивый allocator `job_id/session_id` на microSD;
- неизменяемый snapshot задания;
- отдельный изменяемый runtime-state;
- атомарная запись через временные файлы;
- восстановление последней валидной сессии после перезапуска;
- fail-safe политика без автоматической повторной отправки и без автоматического продолжения;
- ручное закрытие опасного восстановленного состояния через `CLOSED_AFTER_REVIEW`;
- endpoint `POST /api/recovery/acknowledge` с проверкой `session_id` и `confirmed=true`;
- перекрёстная проверка runtime-state и immutable snapshot;
- типизированное чтение `program_type`, `coil_count` и `turns[]`;
- восстановление программы для `/api/status` только для отображения.

Во всех recovery-сценариях:

```text
automatic_queue_allowed = false
automatic_resume_allowed = false
```

Ни один из этих механизмов не управляет SSR и не разрешает физический запуск.

## Хранилище

```text
/data/winding-jobs/id-state.txt
/data/winding-jobs/snapshots/session-<session_id>.json
/data/winding-jobs/state/session-<session_id>.json
```

Snapshot содержит исходное задание и не перезаписывается. Runtime-state содержит текущее состояние доставки и выполнения.

## Текущие ограничения

В immutable snapshot пока явно сохраняются неизвестными:

```text
repair_id: null
motor_id: null
wire_type: null
wire_diameter: null
created_at: null
```

Поэтому задание ещё не имеет строгой связи с конкретным ремонтом, двигателем и выбранной катушкой склада.

Также ещё не выполнены автоматизированные отказные тесты для:

- повреждённого snapshot;
- отсутствующего snapshot при существующем runtime-state;
- несовпадения `job_id/session_id`;
- отказа или заполнения microSD;
- перезапуска до `JOB_ACK`;
- потери ACK после принятия задания Arduino;
- перезапуска после `RUN_STARTED`;
- старого ACK от предыдущего задания.

## Следующий обязательный шаг

1. Подтвердить зелёный `ESP32 Build` для коммита `232151f`.
2. Добавить тестируемый слой сценариев восстановления и отказов хранилища.
3. Найти фактические API/модели текущего ремонта и двигателя.
4. Расширить вход создания задания реальными `repair_id` и `motor_id`.
5. Проверять существование и согласованность ремонта/двигателя до выделения идентификаторов задания.
6. Сохранять проверенные идентификаторы в immutable snapshot.
7. Только после этого проектировать автоматическое списание провода по факту завершённой катушки.

## Запрещённые упрощения

- не использовать `0` вместо неизвестного идентификатора;
- не создавать второй allocator рядом с существующим;
- не доверять runtime-state без snapshot;
- не разрешать новое задание при неполном восстановлении;
- не возобновлять намотку автоматически;
- не управлять SSR со стороны ESP32;
- не списывать материал без устойчивой связи с ремонтом и конкретной катушкой.
