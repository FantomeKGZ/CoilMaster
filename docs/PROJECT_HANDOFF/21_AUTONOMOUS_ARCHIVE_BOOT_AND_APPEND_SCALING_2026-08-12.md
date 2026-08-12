# Autonomous archive boot + append scaling — 2026-08-12

Ветка: `cmp-protocol-v1`

## Цель

Закрыть оставшиеся repeated full-file scan paths автономного Arduino archive после внедрения cursor pagination, не меняя append-only NDJSON format и не вводя DB/rotation без реальных metrics.

## 1. Boot validation переведён на authoritative audit

Раньше:

```text
AutonomousWindingArchive::begin()
→ validateEvents()
→ per-completion matchingStartExists() full scan
→ validateAssignments()
→ per-assignment completedTaskExists() full scan
```

При росте архива boot-time validation мог становиться квадратичным.

Теперь:

```text
AutonomousWindingArchive::begin()
→ ensureDirectories()
→ AutonomousWindingArchive::validateStorage()
```

То есть boot использует тот же bounded-complexity authoritative validator, что и backup/deep-integrity.

Legacy unbounded `appendTasksJson()`, `validateEvents()`, `validateAssignments()`, `containsEvent()` и `latestAssignment()` удалены из production class/code.

## 2. LOCAL_EVT append/replay больше не сканирует весь events.ndjson

До изменения каждый `save()` делал full-file scan для:

```text
findEventReplay()
matchingStartExists()
```

Теперь используется bounded tail lookup последней NDJSON event-record.

Инварианты:

```text
incoming run_id < latest run_id
→ INVALID

incoming run_id > latest run_id
→ разрешён только при session_id >= latest session_id

same run_id + same event
→ DUPLICATE только при exact completed_runs + type/program/session match

same run_id
RUN_STARTED → RUN_COMPLETED
→ допустимый transition

same run_id с другим session/program/type ordering
→ INVALID
```

Это сохраняет FIFO/replay semantics Arduino transport и делает normal append/retry latency независимой от полного размера history.

`save()` дополнительно сам fail-closed проверяет:

```text
RUN_STARTED   -> completed_runs == 0
RUN_COMPLETED -> completed_runs > 0
```

даже если caller уже прошёл UART parser validation.

## 3. Tail lookup bounded RAM / I/O

Последняя event-record читается максимум из последних 512 bytes файла.

Generated event record значительно меньше этого лимита. Если tail не содержит целой terminated NDJSON record, lookup fail-closed возвращает storage/integrity failure вместо append поверх неоднозначного tail.

## 4. Power-loss NDJSON termination

Authoritative `validateStorage()` теперь требует, чтобы непустые:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

заканчивались `\n`.

Это закрывает edge, где после interrupted append содержимое могло закончиться syntactically valid JSON object ровно перед newline и раньше пройти `readStringUntil('\n')` как полноценная запись.

## 5. Что сознательно не менялось

Остаются:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

Не добавлены:

- database migration;
- destructive compaction;
- automatic rotation;
- automatic physical START;
- auto-resume;
- automatic wire writeoff;
- изменение exact spool/session/run safety semantics.

`completedTaskExists()` и assignment validation пока остаются full-scan там, где оператор может привязать произвольную старую задачу. Это редкая storage-boundary операция и сохраняет full historical integrity check. Оптимизировать её до появления реальных latency metrics не требуется.

## Commits

```text
bff50bda95eebd99c4d49fcb5e6ec8f55a9df337  Remove legacy autonomous archive scan API
a9bfacbc508651b9dea9f81550068add517474d1  Use bounded autonomous audit at boot
51f1ecc9f2c5f980db909e61e87829279b0a9e92  Add bounded autonomous tail lookup
61c456d283c3cca31fd6aa5d0d4e033aedf6be32  Bound autonomous event replay lookup
e534e9bb9a670ebe42ebc51f822c15bbc6b08d11  Reject unterminated autonomous NDJSON records
```

## Verification status

Эти firmware изменения сделаны после последнего подтверждённого ESP32 build.

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

Нужен clean ESP32 build перед следующим firmware block.

## После build

После `SUCCESS` и upload:

1. проверить обычный local Arduino START/COMPLETE и `LOCAL_EVT`;
2. повторный exact frame должен остаться `DUPLICATE`;
3. `/api/autonomous-windings?limit=1` должен продолжать работать по cursor;
4. `/api/backup/manifest` должен оставаться `snapshot_stable=true` на intact storage;
5. снять `autonomous_winding_archive_audit_duration_ms` на уже накопленной history.

Следующий performance decision — только по реальным size/count/timing metrics. Rotation threshold заранее не вводить.
