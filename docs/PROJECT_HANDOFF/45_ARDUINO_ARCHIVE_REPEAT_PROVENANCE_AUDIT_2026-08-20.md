# CoilMaster — Arduino archive repeat provenance audit

Дата: **2026-08-20**  
Ветка: `cmp-protocol-v1`

## Результат

Аудит planned repeat provenance для автономного Arduino archive завершён.

Главный вывод:

> Для `localStandalone` Arduino winding archive сейчас **нет authoritative planned `repeat_target`**. UI должен показывать `—`, а не выводить значение из `completed_runs`, `coil_count`, history или default `1`.

Изменять UART/archive schema только ради заполнения этого поля сейчас не требуется и небезопасно с точки зрения provenance.

## Проверенные пути

### ESP32 autonomous archive

`CM_AutonomousWindingArchive` получает только события, для которых:

```text
event.localStandalone == true
```

Remote/Web winding JOB lifecycle обрабатывается отдельно.

Поэтому `CM_JobSnapshotStore` нельзя использовать как источник planned repeat для autonomous archive.

### Remote Web JOB

`OutgoingWindingJob` / job snapshot содержат explicit:

```text
repeatTarget
```

Это authoritative planned repeat для Web/ESP32 JOB path.

Он не относится автоматически к local standalone Arduino run.

### Arduino LOCAL_EVT

Текущий local UART payload передаёт:

```text
LOCAL_EVT
session_id
run_id
event
completed_runs
winding_type
coil_count
turns/program
```

Planned `repeat_target` в payload отсутствует.

### Local WindingJob default

В `Core/CM_WindingJob.h` поле `repeatTarget` существует с default `1`, но completion semantics `repeatTargetReached()` применяются только когда:

```text
source == WindingJobSource::Esp32Web
```

Следовательно, local default `1` нельзя публиковать как доказанный planned repeat автономной намотки.

### Persisted autonomous archive

Текущая append-only archive schema сохраняет:

```text
session_id
run_id
event/completion state
completed_runs
winding_type
coil_count
program/turns
start_observed
```

и не сохраняет `repeat_target`.

Для существующих records это корректно: отсутствующее planned metadata остаётся неизвестным и не должно быть задним числом выдумано.

## Принятое поведение UI

Arduino archive показывает:

```text
planned repeats = explicit API repeat_target, если он когда-либо станет доступен
planned repeats = —, если authoritative metadata отсутствует
```

Это закреплено в:

```text
Tests/Web/check_arduino_archive_ui.js
```

## Что НЕ делать

Не допускается:

- подмешивать `JobSnapshotStore.repeatTarget` в `localStandalone` archive;
- считать `completed_runs` planned target;
- считать historical run count planned target;
- считать `coil_count` planned target;
- публиковать local `WindingJob.repeatTarget == 1` как доказанный operator plan;
- переписывать старые immutable RUN records для заполнения неизвестного metadata.

## Когда repeat_target можно будет добавить

Если в будущем появится explicit local Arduino UX для задания planned repeats, это должен быть отдельный semantic/protocol feature:

1. explicit operator-defined local planned repeat value;
2. привязка к exact local job/session provenance;
3. UART transport этого metadata;
4. backward-compatible archive schema extension;
5. old records остаются без значения;
6. отсутствие automatic physical START между repeat cycles сохраняется.

До этого момента planned repeat в autonomous archive остаётся unknown.

## Следующий repo-level приоритет

Переход к kg-first material consumption audit/migration:

```text
quantity_kg authoritative
exact source_session_id + source_run_id
immutable material/conductor snapshot
spool_id optional для нового режима
exact stock decrement только при наличии spool
unallocated/manual consumption без spool
RUN_COMPLETED никогда не делает automatic writeoff
legacy exact spool provenance сохраняется
```
