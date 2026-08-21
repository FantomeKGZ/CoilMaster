# CoilMaster — продолжение проекта

Дата обновления: **2026-08-21**

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

1. `60_PROTOCOL_CI_RECOVERY_2026-08-21.md` — `CMP Protocol Tests #2170` подтверждён зелёным на `2c00b2c8`; stale Web/safety contracts восстановлены, workflow теперь показывает все audit failures за один run.
2. `59_BACKUP_SESSION_PREFLIGHT_INTEGRATION_2026-08-21.md` — duplicate manifest session-directory preflight удалён; backup manifest теперь использует authoritative classified/measured `WindingSessionPersistenceIntegrityAudit`.
3. `58_BACKUP_SESSION_PREFLIGHT_CONSOLIDATION_2026-08-21.md` — read-only classified session preflight перенесён внутрь authoritative persistence audit до любых store `begin()`.
4. `57_KG_FIRST_BACKUP_WAREHOUSE_INTEGRITY_2026-08-21.md` — kg-first warehouse/material persistence включены в deep backup integrity.
5. `56_COSTING_SINGLE_PASS_WAREHOUSE_AGGREGATION_2026-08-21.md` — warehouse costing aggregation переведена на bounded/single-pass путь.
6. `55_WAREHOUSE_WINDING_BOUNDED_SCAN_HARDENING_2026-08-21.md` — bounded scan hardening для warehouse/winding provenance.
7. `54_WRITEOFF_FAULT_HTTP_AND_PROVENANCE_SCALING_2026-08-21.md` — fault HTTP semantics и provenance scaling writeoff path.
8. `53_WRITEOFF_HARDWARE_FAULT_INJECTION_2026-08-21.md` и `52_WRITE_OFF_FAULT_PATH_ACCEPTANCE_2026-08-21.md` — writeoff fault-path acceptance/hardware injection checkpoints.
9. `51_NDJSON_GROWTH_OBSERVABILITY_2026-08-20.md` — observability растущих NDJSON без преждевременной миграции в БД.
10. `50_KG_FIRST_COSTING_COMPATIBILITY_2026-08-20.md`, `49_KG_FIRST_WRITEOFF_UI_2026-08-20.md`, `48_KG_FIRST_BACKEND_ACTIVATION_2026-08-20.md`, `47_KG_FIRST_QUANTITY_FOUNDATION_2026-08-20.md`, `46_KG_FIRST_WAREHOUSE_STORAGE_API_AUDIT_2026-08-20.md` — реализованный kg-first material flow.
11. `45_ARDUINO_ARCHIVE_REPEAT_PROVENANCE_AUDIT_2026-08-20.md` и checkpoints `39–44` — repeat/archive/motor/Hall/JOB cancel baseline.
12. `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md` и более старые checkpoints — предыдущий подтверждённый release/hardware baseline; не доказательство текущего HEAD.

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
- legacy exact spool provenance сохраняется;
- linked immutable snapshot/history не удаляется operational cancellation;
- backup restore operator-only, transactional, fail-closed;
- deep backup validation остаётся read-only/fail-closed;
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

Desktop/mobile motor catalogs показывают phases/slots/repeats, поддерживают numeric search и раздельные действия `Подробнее` / `Выбрать для ремонта`.

Motor detail pages:

```text
/desktop/motor-details.html?motor_id=<id>
/mobile/motor-details.html?motor_id=<id>
```

Backend repair-history endpoint:

```text
GET /api/motors/repairs?motor_id=<id>&cursor=<optional>&limit=<optional>&status=<optional>
```

## Arduino winding archive

Реализованы:

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

Autonomous `localStandalone` archive не имеет authoritative planned `repeat_target`. Не подмешивать Web JOB snapshot, `completed_runs`, `coil_count` или local default `1`; unknown показывать как `—`. Старые immutable RUN records не переписывать.

## JOB cancel/recovery

Ключевое поведение:

- no-run pending/accepted remote JOB может быть safely cancelled;
- Arduino cancel идемпотентна для already-clear state;
- physical fallback `D → * → # → D` отправляет `ALL_CLEAR`;
- reboot recovery не создаёт auto-start, RUN_COMPLETED или wire writeoff.

Подробно: checkpoint `39_JOB_CANCEL_RECOVERY_2026-08-18.md`.

## Kg-first material consumption — реализованная линия

Checkpoints `46–50` перевели новый material consumption на kg-first semantics:

```text
quantity_kg authoritative для нового consumption
exact source_session_id + source_run_id сохраняются
immutable material/conductor provenance сохраняется
spool_id optional в KG_FIRST mode
exact stock decrement выполняется только при наличии spool
unallocated/manual consumption допустим без spool
legacy exact-spool provenance остаётся совместимым
RUN_COMPLETED никогда не списывает провод автоматически
```

Следующие checkpoints `52–57` усилили fault paths, bounded provenance scans, costing aggregation и backup integrity.

## Backup session persistence — завершённый recovery block

`WindingSessionPersistenceIntegrityAudit` является authoritative owner read-only preflight для:

```text
/data/winding-jobs/snapshots
/data/winding-jobs/state
/data/winding-jobs/spool-selection
```

До любых store `begin()` он классифицирует:

```text
DirectoryUnavailable
TemporaryFilePresent
InvalidDirectoryEntry
ContentInvalid
None
```

Backup manifest больше не выполняет второй preflight scan. Он использует те же measured/classified результаты audit и сохраняет внешние reason strings:

```text
session_directory_unavailable
session_temp_present
session_directory_invalid
winding_session_persistence_unstable_or_invalid
```

`scanSessionDirectory()` при этом остаётся для bounded `/api/backup/sessions` enumeration; его не удалять как якобы duplicate helper.

Подробно: checkpoints `58` и `59`.

## Verification status текущего HEAD

Подтверждённый CI baseline recovery:

```text
2c00b2c8d57e4f4dd0806ac29be7f24893ebf2c2  Fix write-off fault error token contract
CMP Protocol Tests #2170                           GREEN
```

В `#2170` зелёным завершился весь `.github/workflows/cmp-protocol-tests.yml`: host protocol/state-machine tests и все настроенные Web/safety audits. Подробно: checkpoint `60_PROTOCOL_CI_RECOVERY_2026-08-21.md`.

Следующие documentation commits после этого baseline не изменяют production firmware semantics, но отдельные build workflows всё равно проверять по их собственным runs.

Без нового фактического результата не считать подтверждёнными:

```text
pio run -e uno / Arduino Uno Build current HEAD
pio run -e esp32 / ESP32 Build current HEAD
hardware UART E2E current HEAD
hardware acceptance current HEAD
```

Старые release/build successes — только историческое доказательство прежнего baseline.

## Текущий recovery-приоритет

Protocol CI восстановлен и больше не является blocker.

Продолжать от текущего HEAD:

```text
1. разобрать отдельно падающий ESP32 Build и исправить подтверждённые compile/link/size regressions;
2. проверить Arduino Uno Build текущего HEAD и recent Flash/SRAM footprint commits на functional regression;
3. затем focused source-level audit изменений после последнего hardware-accepted baseline;
4. найденные подтверждённые ошибки исправлять сразу с current blob SHA + regression contract;
5. hardware acceptance текущего HEAD проводить отдельно — CI не заменяет hardware E2E.
```

## Короткий текст для нового чата

```text
Продолжаем CoilMaster recovery.
Repo FantomeKGZ/CoilMaster, source-of-truth cmp-protocol-v1, main не использовать.
Сначала прочитай docs/PROJECT_HANDOFF/00_READ_FIRST.md,
60_PROTOCOL_CI_RECOVERY_2026-08-21.md,
59_BACKUP_SESSION_PREFLIGHT_INTEGRATION_2026-08-21.md,
58_BACKUP_SESSION_PREFLIGHT_CONSOLIDATION_2026-08-21.md,
57_KG_FIRST_BACKUP_WAREHOUSE_INTEGRITY_2026-08-21.md
и checkpoints 46–56 по kg-first/writeoff recovery.
Перед каждым изменением existing file fetch current blob SHA.
Не заявляй build/CI green без фактического результата.
CMP Protocol Tests #2170 на 2c00b2c8 подтверждён GREEN.
Текущий приоритет: ESP32 Build recovery, затем Arduino Uno Build/Flash-SRAM audit и hardware E2E.
```
