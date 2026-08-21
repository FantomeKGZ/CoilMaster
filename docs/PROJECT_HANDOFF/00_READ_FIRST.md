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

1. `59_BACKUP_SESSION_PREFLIGHT_INTEGRATION_2026-08-21.md` — duplicate manifest session-directory preflight удалён; backup manifest теперь использует authoritative classified/measured `WindingSessionPersistenceIntegrityAudit`.
2. `58_BACKUP_SESSION_PREFLIGHT_CONSOLIDATION_2026-08-21.md` — read-only classified session preflight перенесён внутрь authoritative persistence audit до любых store `begin()`.
3. `57_KG_FIRST_BACKUP_WAREHOUSE_INTEGRITY_2026-08-21.md` — kg-first warehouse/material persistence включены в deep backup integrity.
4. `56_COSTING_SINGLE_PASS_WAREHOUSE_AGGREGATION_2026-08-21.md` — warehouse costing aggregation переведена на bounded/single-pass путь.
5. `55_WAREHOUSE_WINDING_BOUNDED_SCAN_HARDENING_2026-08-21.md` — bounded scan hardening для warehouse/winding provenance.
6. `54_WRITEOFF_FAULT_HTTP_AND_PROVENANCE_SCALING_2026-08-21.md` — fault HTTP semantics и provenance scaling writeoff path.
7. `53_WRITEOFF_HARDWARE_FAULT_INJECTION_2026-08-21.md` и `52_WRITE_OFF_FAULT_PATH_ACCEPTANCE_2026-08-21.md` — writeoff fault-path acceptance/hardware injection checkpoints.
8. `51_NDJSON_GROWTH_OBSERVABILITY_2026-08-20.md` — observability растущих NDJSON без преждевременной миграции в БД.
9. `50_KG_FIRST_COSTING_COMPATIBILITY_2026-08-20.md`, `49_KG_FIRST_WRITEOFF_UI_2026-08-20.md`, `48_KG_FIRST_BACKEND_ACTIVATION_2026-08-20.md`, `47_KG_FIRST_QUANTITY_FOUNDATION_2026-08-20.md`, `46_KG_FIRST_WAREHOUSE_STORAGE_API_AUDIT_2026-08-20.md` — реализованный kg-first material flow.
10. `45_ARDUINO_ARCHIVE_REPEAT_PROVENANCE_AUDIT_2026-08-20.md` и checkpoints `39–44` — repeat/archive/motor/Hall/JOB cancel baseline.
11. `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md` и более старые checkpoints — предыдущий подтверждённый release/hardware baseline; не доказательство текущего HEAD.

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

## Backup session persistence — текущий завершённый recovery block

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

На момент обновления `00_READ_FIRST.md` последние recovery commits:

```text
a141f7d5fcf9216a178ca31dbefc6189638f8e22  Consolidate backup session preflight
9d8e3799422fcd6bd38c4a8e89fe6c1f45ad7289  Guard consolidated backup session preflight
53052f2279631675c125c780ceed85379a433ec1  Checkpoint consolidated backup session preflight
```

GitHub workflow definitions существуют и `CMP Protocol Tests` настроен на `push` в `cmp-protocol-v1`, включая `Tests/Web/**` и `firmware/esp32/src/**`. Однако connector не показал status checks/workflow runs для recovery HEAD.

Поэтому без нового фактического результата не считать подтверждёнными:

```text
pio run -e uno
pio run -e esp32
CMP Protocol Tests current HEAD
hardware UART E2E current HEAD
GitHub CI current HEAD
```

Старые release/build successes — только историческое доказательство прежнего baseline.

## Текущий recovery-приоритет

Kg-first material flow и backup session-preflight integration уже не являются next task.

Продолжать от текущего HEAD:

```text
1. проверить оставшиеся build/test regressions после recent Uno Flash/SRAM recovery;
2. проверить recent Uno footprint commits на functional regression, не откатывая safety semantics;
3. затем focused source-level audit изменений после последнего hardware-accepted baseline;
4. найденные подтверждённые ошибки исправлять сразу с current blob SHA + regression contract;
5. hardware acceptance текущего HEAD проводить отдельно — старый hardware pass не переносить автоматически.
```

## Короткий текст для нового чата

```text
Продолжаем CoilMaster recovery.
Repo FantomeKGZ/CoilMaster, source-of-truth cmp-protocol-v1, main не использовать.
Сначала прочитай docs/PROJECT_HANDOFF/00_READ_FIRST.md,
59_BACKUP_SESSION_PREFLIGHT_INTEGRATION_2026-08-21.md,
58_BACKUP_SESSION_PREFLIGHT_CONSOLIDATION_2026-08-21.md,
57_KG_FIRST_BACKUP_WAREHOUSE_INTEGRITY_2026-08-21.md,
56_COSTING_SINGLE_PASS_WAREHOUSE_AGGREGATION_2026-08-21.md,
55_WAREHOUSE_WINDING_BOUNDED_SCAN_HARDENING_2026-08-21.md
и checkpoints 46–54 по kg-first/writeoff recovery.
Перед каждым изменением existing file fetch current blob SHA.
Не заявляй build/CI green без фактического результата.
Текущий приоритет: build/test recovery после recent Uno Flash/SRAM fixes,
затем focused audit изменений после последнего hardware-accepted baseline.
```
