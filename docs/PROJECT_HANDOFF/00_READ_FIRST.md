# CoilMaster — продолжение проекта

Дата обновления: **2026-08-20**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`. `main` для исходников не использовать.

## Для AI / coding agent

Перед широким поиском читать maintenance-layer:

```text
/AGENTS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/03_ADD_MODULE_PLAYBOOK.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Перед изменением существующего файла обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить, что exact path отсутствует. Не утверждать CI/build green без фактического результата.

## Текущие главные checkpoints

Читать в таком порядке:

1. `45_ARDUINO_ARCHIVE_REPEAT_PROVENANCE_AUDIT_2026-08-20.md` — authoritative planned `repeat_target` для `localStandalone` archive отсутствует; unknown metadata не угадывать и не подмешивать из Web JOB snapshot.
2. `44_ARDUINO_ARCHIVE_UI_REDESIGN_2026-08-20.md` — compact desktop/mobile Arduino archive, multi-select, bulk create/link/combine, on-demand historical RUN counts.
3. `43_MOTOR_SCHEMA_AND_DETAILS_IMPLEMENTATION_2026-08-20.md` — новая motor schema/UI, detail pages, exact motor repair history и regression contracts.
4. `42_REPEAT_TARGET_JOB_LIFECYCLE_IMPLEMENTATION_2026-08-20.md` — семантика `program + repeat_target`, один RUN на полный program cycle и final JOB auto-clear после ACK/DUPLICATE.
5. `41_HALL_AUTOCALIBRATION_ACCEPTED_2026-08-20.md` — утверждённая automatic Hall calibration с обязательным physical START.
6. `40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md` — общий roadmap: hardware settings, motors, Arduino archive, kg-first materials и common web UX.
7. `39_JOB_CANCEL_RECOVERY_2026-08-18.md` — resilient JOB_CANCEL / ALL_CLEAR / reboot recovery.
8. `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md` и более старые checkpoints — предыдущий подтверждённый baseline, не доказательство текущего HEAD.

Текущий код `cmp-protocol-v1` имеет приоритет над историческими checkpoints.

## Аппаратный справочник

```text
docs/HARDWARE_REFERENCE/00_READ_FIRST.md
docs/HARDWARE_REFERENCE/01_ARDUINO_CONNECTIONS.md
docs/HARDWARE_REFERENCE/02_ESP32_CONNECTIONS.md
docs/HARDWARE_REFERENCE/03_KEYS_AND_HIDDEN_COMMANDS.md
docs/HARDWARE_REFERENCE/04_RTC_TIME_SYNC.md
docs/HARDWARE_REFERENCE/07_HALL_SENSOR_CALIBRATION.md
```

Hall factory baseline:

```text
A0
threshold 590
hysteresis 50
release threshold 540
```

Stable-release защита против repeated count при зависшем магните уже добавлена. Automatic calibration утверждена, но её hardware acceptance не переносить автоматически на текущий HEAD.

## Safety-инварианты

Нельзя менять:

- physical START только физический;
- никакого automatic physical START между repeat cycles;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся explicit/manual и сохраняет exact `source_session_id + source_run_id`;
- legacy exact spool provenance не уничтожать при kg-first migration;
- linked immutable snapshot/history не удаляется operational cancellation;
- backup restore operator-only, transactional, fail-closed;
- hardware settings менять только при доказанном safe idle;
- Web может подготовить Hall calibration, но двигатель запускает только physical START;
- calibration mode не создаёт production RUN events/history/writeoff;
- SSR OFF обязателен при calibration success/timeout/fault/abort;
- рекомендуемые Hall settings не сохраняются без explicit Apply.

## Реализованная winding semantics

```text
38/38 × 6
```

означает:

```text
program = [38, 38]
repeat_target = 6
```

Это **один JOB**. Один полный проход `38/38` — **один RUN и один run_id**. После каждого полного прохода Arduino ждёт новый physical START. Между повторами automatic START отсутствует.

Различать:

```text
repeat_target     — planned repeats текущего JOB
completed_runs    — completed repeats текущего JOB
historical_runs   — историческое число фактических runs
coil_count        — отдельная физическая характеристика, если нужна
```

После final repeat Arduino очищает active remote JOB только после ACK/DUPLICATE exact final `RUN_COMPLETED`. History остаётся immutable.

Repo-level regression `38/38 × 6` добавлен. Host StateMachine test ранее был фактически собран локально с `-Wall -Wextra -Wpedantic -Werror` и прошёл. Полные PlatformIO builds текущего HEAD всё ещё требуют отдельного подтверждения.

## Реализованная motor schema

Предпочтительные поля:

```text
manufacturer
model
phase_count
slot_count
coil_program
repeat_target
```

Совместимость:

- persisted legacy `phases` сохранён;
- API принимает `phase_count`, legacy `phases` остаётся совместимым;
- conflicting phase values отклоняются;
- `repeat_target` хранится явно, диапазон `1..65535`;
- новые records default `repeat_target=1`;
- отсутствующие legacy `phase/slot/repeat` metadata показываются `не указано`, не выводятся из догадок;
- legacy `name` сохранён, но основной UI title — `manufacturer + model`.

Desktop/mobile motor catalogs уже показывают phases/slots/repeats, поддерживают numeric search и раздельные действия `Подробнее` / `Выбрать для ремонта`.

## Motor detail pages и repair history

Добавлены:

```text
/desktop/motor-details.html?motor_id=<id>
/mobile/motor-details.html?motor_id=<id>
```

Они показывают identity, winding program/repeats, electrical/mechanical/winding metadata, source/confidence/comment и bounded repair history.

Backend endpoint:

```text
GET /api/motors/repairs?motor_id=<id>&cursor=<optional>&limit=<optional>&status=<optional>
```

Он фильтрует repair journal по exact motor_id на ESP32 и возвращает bounded pagination, поэтому browser не сканирует весь journal.

## Arduino winding archive redesign

Первый UI этап реализован:

```text
compact desktop table
compact mobile rows/cards
multi-select completed RUN records
bulk link/create/combine motor actions
status badges + accessible details
on-demand historical completed RUN count by program
```

Shared controller:

```text
firmware/esp32/web/shared/arduino-windings-archive.js
```

Linkage не меняет immutable RUN events и exact `session_id + run_id`.

Planned repeat provenance audit закрыт: autonomous archive относится к `localStandalone`, а Web JOB snapshot — к другому lifecycle. Текущий local UART/archive path не содержит authoritative planned `repeat_target`; local default `1` не является доказанным operator plan. Поэтому planned repeats показываются `—`, если explicit API metadata отсутствует. Старые RUN records не переписываются.

Подробно: checkpoints `44` и `45`.

## JOB cancel/recovery

Ключевое поведение остаётся:

- no-run pending/accepted remote JOB может быть safely cancelled;
- Arduino cancel идемпотентна для already-clear state;
- physical fallback `D → * → # → D` отправляет `ALL_CLEAR`;
- reboot recovery не создаёт auto-start, RUN_COMPLETED или wire writeoff.

Подробно: checkpoint `39`.

## Verification status текущего HEAD

Не считать подтверждёнными без отдельного результата:

```text
pio run -e uno
pio run -e esp32
CMP Protocol Tests current HEAD
hardware UART E2E current HEAD
```

Старые release/build successes остаются историческим доказательством прежнего baseline, но не нового motor/repeat/Hall/archive блока.

## Следующий repo-level приоритет

Planned repeat provenance audit для autonomous archive закрыт без ложного schema extension.

Текущий приоритет:

```text
kg-first material consumption
→ quantity_kg authoritative/required для нового consumption
→ exact source_session_id + source_run_id
→ immutable material/conductor snapshot
→ spool_id optional для нового режима
→ exact stock decrement only when spool exists
→ unallocated/manual consumption when no spool
→ legacy exact spool provenance сохраняется
→ RUN_COMPLETED still never auto-writes off wire
```

После этого common web UX: FTP page shell, common app shell, Asia/Bishkek clock everywhere, unified status/toast/search/navigation.

## Короткий текст для нового чата

```text
Продолжаем CoilMaster.
Repo FantomeKGZ/CoilMaster, source-of-truth cmp-protocol-v1, main не использовать.
Прочитай docs/PROJECT_HANDOFF/00_READ_FIRST.md,
45_ARDUINO_ARCHIVE_REPEAT_PROVENANCE_AUDIT_2026-08-20.md,
44_ARDUINO_ARCHIVE_UI_REDESIGN_2026-08-20.md,
43_MOTOR_SCHEMA_AND_DETAILS_IMPLEMENTATION_2026-08-20.md,
42_REPEAT_TARGET_JOB_LIFECYCLE_IMPLEMENTATION_2026-08-20.md,
41_HALL_AUTOCALIBRATION_ACCEPTED_2026-08-20.md и
40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md.
Перед каждым изменением existing file fetch current blob SHA.
Не заявляй build/CI green без фактической проверки.
Текущий repo-level приоритет: kg-first material consumption,
затем common web UX.
```
