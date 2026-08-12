# Workshop registry scaling — 2026-08-12

Ветка: `cmp-protocol-v1`

## Цель блока

Убрать следующий класс growth-risk после autonomous archive: unbounded workshop list responses и O(n^2) boot-integrity для clients / motors / repairs.

Формат хранения не меняется:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
/data/repairs/pricing.ndjson
```

Нет DB migration, destructive compaction или automatic rotation.

## 1. Autonomous assignment path

До блока один manual assignment выполнял completed-task lookup дважды:

```text
HTTP completedTaskExists()
→ assignMotor()
→ completedTaskExists() again
```

Добавлен `assignMotorChecked()` с typed result. Теперь `events.ndjson` сканируется один раз на assignment, а HTTP сохраняет отдельные semantics для not-found / integrity / storage / write failure.

Полный scan `assignments.ndjson` для следующего `assignment_id` пока сохранён намеренно как runtime integrity boundary. Отдельный persisted assignment high-water не вводится без populated-dataset metrics.

## 2. Bounded workshop list readers

Добавлены:

```text
RepairRegistry::appendClientsPageJson()
RepairRegistry::appendMotorsPageJson()
RepairRegistry::appendRepairsPageJson()
```

Параметры API при paged request:

```text
limit default 20 when paging requested
limit max 32
cursor opaque byte offset
has_more
next_cursor
max_page_size
```

Cursor принимается только на NDJSON record boundary.

Existing filters сохранены:

```text
clients: phone
motors: q / name
repairs: client_id
```

Legacy GET без `limit/cursor` пока оставлен для совместимости с ещё не мигрированными экранами. Поэтому bounded mode на этих трёх endpoints ещё не является обязательным default.

## 3. UI, уже переведённые на paged registry reads

```text
mobile/clients.html
desktop/clients.html
mobile/motors.html
desktop/motors.html
mobile/repairs.html
desktop/repairs.html
mobile/arduino-windings.html
desktop/arduino-windings.html
```

Когда экрану нужен полный catalog для select/search, browser агрегирует последовательные pages по 32. ESP32 при этом не строит один JSON размером со всю базу.

После firmware upload для этого блока необходимо полностью заменить microSD `/web` актуальным `firmware/esp32/web`.

## 4. Exact by-ID registry lookup

Добавлены read-only endpoints:

```text
GET /api/clients/by-id?client_id=...
GET /api/motors/by-id?motor_id=...
GET /api/repairs/by-id?repair_id=...
```

Response:

```json
{"item":{}}
```

Missing identity -> 404.
Invalid identity -> 400.
Registry unavailable -> 503.
Integrity failure -> 500.

Storage methods reject duplicate matching identity rather than silently selecting one record.

Mobile costing уже переведён на exact repair -> client/motor lookup и больше не перечисляет три полных registries ради одной карточки.

Desktop costing на этом checkpoint ещё использует legacy full registry GET и должен быть мигрирован перед удалением legacy list mode.

## 5. Authoritative business-data integrity audit

Старый `BackupBusinessDataIntegrityAudit` имел repeated exact-one-match scans и был O(n^2).

Новый audit использует:

```text
clients IDs      strict monotonic append-only pass
motors IDs       strict monotonic pass + coil_program validation
repairs IDs      strict monotonic pass
repair refs      batches of 32 client/motor references
CLOSED status    batches of 32 + exact-one status occurrence + repair reference
pricing refs     batches of 32 repair references
```

Non-empty NDJSON files должны заканчиваться `\n`; interrupted append fail-closed.

`checkWorkshopRegistry()` валидирует clients/motors/repairs/status без pricing dependency.

Full backup `check()` выполняет тот же workshop audit и затем pricing audit.

## 6. RepairRegistry boot and new ID allocation

`RepairRegistry::begin()` больше не запускает legacy nested validators. Он вызывает authoritative:

```text
BackupBusinessDataIntegrityAudit::checkWorkshopRegistry()
```

`nextId()` больше не выполняет `validateUniqueIds()` O(n^2). Он одним строгим проходом проверяет monotonic append-only IDs и выдаёт `last_id + 1`.

Старые private:

```text
validateUniqueIds()
validateRepairStatusHistory()
```

удалены из `RepairRegistry`.

## Safety invariants

Не менялись:

- physical START только физический;
- ESP32/Web не управляют SSR;
- нет auto-resume;
- `RUN_COMPLETED` не списывает провод автоматически;
- списание провода остаётся manual exact spool/session/run;
- repair close finalization semantics не изменены.

## Verification status

Все firmware/UI изменения этого блока сделаны после последнего подтверждённого ESP32 build.

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

Перед дальнейшим крупным firmware refactor нужен clean ESP32 build.

## Следующее после build

1. Мигрировать desktop costing и оставшиеся legacy consumers на exact/paged registry reads.
2. После этого сделать clients/motors/repairs bounded mode default и удалить legacy unbounded formatter path.
3. Проверить `repairClosed()`/repair list status lookup: сейчас bounded page ограничивает количество repair rows, но каждый row всё ещё может сканировать status ledger. Оптимизировать только после build и compile-safety текущего batch.
4. Снять populated-dataset timings через backup manifest.
5. Rotation/segmentation thresholds вводить только по фактическим latency/size metrics.
