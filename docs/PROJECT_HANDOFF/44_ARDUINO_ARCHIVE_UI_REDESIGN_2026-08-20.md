# CoilMaster — Arduino archive UI redesign

Дата: **2026-08-20**  
Ветка: `cmp-protocol-v1`

## Статус

Первый repo-level этап redesign Arduino winding archive реализован для desktop/mobile UI без изменения immutable physical RUN history.

## Что изменено

Добавлен общий controller:

```text
firmware/esp32/web/shared/arduino-windings-archive.js
```

Обновлены страницы:

```text
firmware/esp32/web/desktop/arduino-windings.html
firmware/esp32/web/mobile/arduino-windings.html
```

Теперь архив использует:

- компактную desktop table вместо повторяющихся больших карточек;
- компактные mobile rows/cards;
- multi-select checkboxes;
- session/run ID;
- program;
- planned repeat column/field;
- completed run count;
- optional on-demand historical run count by exact normalized program;
- status badges;
- motor linkage state;
- accessible desktop details via hover + keyboard focus;
- accessible mobile details through `<details>`.

## Bulk actions

Сверху архива теперь доступны:

- link selected completed RUN records to an existing motor;
- create a motor from selected records;
- combine several selected programs into one new motor linkage set;
- select all completed rows from the currently loaded page set;
- clear selection.

Незавершённая запись `STARTED_NOT_COMPLETED` не может быть выбрана для linkage как выполненная.

Linkage выполняется через существующий endpoint:

```text
POST /api/autonomous-windings/assign
```

Создание motor record использует существующий:

```text
POST /api/motors
```

Исходные `RUN_STARTED`, `RUN_COMPLETED`, recovered evidence и exact `session_id + run_id` не переписываются и не удаляются.

## Historical actual runs

Обычная загрузка страницы остаётся bounded и не делает второй полный scan архива.

Исторический счёт по программе вычисляется только по explicit operator action:

```text
Посчитать историю программ / Счёт истории
```

Controller читает `/api/autonomous-windings` страницами до 32 logical tasks и считает только records со status `COMPLETED` по exact normalized program.

Это read-only operation; archive storage не меняется.

## Planned repeat target — важное ограничение текущего archive API

Текущий `RemoteWindingEvent` содержит:

```text
completedRuns
```

но не содержит `repeatTarget`.

`repeatTarget` существует у outgoing `OutgoingWindingJob`, но текущий autonomous archive event serializer его не хранит и не публикует.

Поэтому UI показывает planned repeats только если API явно вернул `repeat_target`; иначе показывает:

```text
—
```

Значение нельзя выводить из `completed_runs`, `coil_count` или исторического количества RUN. Это специально закреплено regression contract, чтобы UI не создавал ложную provenance.

Следующий backend substep для planned repeats допустим только после определения authoritative persisted source, связанного с exact JOB/session/run без переписывания старой истории.

## Combine semantics текущего этапа

Motor schema по-прежнему имеет один основной `coil_program`.

При `Объединить программы`:

- создаётся один motor record;
- основной `coil_program` остаётся первой выбранной программой для backward compatibility;
- все выбранные exact `session/run` records link к этому motor;
- полный список выбранных programs и exact runs сохраняется в create comment.

Если требуется first-class multi-program motor schema, это должен быть отдельный backward-compatible schema block, а не скрытое изменение archive UI.

## Regression contract

Добавлен:

```text
Tests/Web/check_arduino_archive_ui.js
```

и включён в:

```text
.github/workflows/cmp-protocol-tests.yml
```

Audit фиксирует:

- общий shared controller для desktop/mobile;
- compact table/mobile details;
- bulk link/create/combine controls;
- exact archive/motor endpoints;
- session/run provenance fields;
- disabled linkage for incomplete RUN;
- immutable RUN wording;
- отсутствие guessed `repeat_target`.

## Verification status

Repo-level implementation и static contract wiring выполнены.

На текущем HEAD пока не считать подтверждёнными без фактического результата:

```text
pio run -e uno
pio run -e esp32
CMP Protocol Tests workflow result
hardware ESP32/Arduino E2E
```

## Следующий приоритет

1. Проверить, есть ли authoritative persisted source для planned `repeat_target` в autonomous archive path. Если нет — спроектировать backward-compatible metadata linkage, не переписывая старые RUN records.
2. После закрытия archive redesign перейти к kg-first material consumption audit/migration:
   - `quantity_kg` authoritative;
   - exact `source_session_id + source_run_id`;
   - immutable material/conductor snapshot;
   - optional `spool_id` для нового режима;
   - no automatic writeoff from `RUN_COMPLETED`.
3. Затем common web UX/app shell/time/status layer.
