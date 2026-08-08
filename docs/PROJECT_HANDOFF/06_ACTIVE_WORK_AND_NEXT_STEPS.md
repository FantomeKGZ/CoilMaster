# Где остановились и что делать дальше

Дата обновления: 2026-08-08
Ветка: `cmp-protocol-v1`
Ориентировочная функциональная готовность: около 90%

## Точная точка продолжения

Основной путь ремонта сейчас замкнут:

```text
клиент
→ двигатель
→ ремонт OPEN
→ калькуляция
→ linked winding
→ обязательный exact spool_id
→ immutable snapshot + immutable spool selection
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ winding history
→ ручное списание провода по фактическому весу
→ source_session_id + source_run_id provenance
→ дополнительные материалы / pricing
→ read-only finalization preflight
→ CLOSED
→ read-only архив
→ месячный отчёт
```

## Что нового закрыто

### Exact spool identity

Новые linked jobs требуют конкретную активную CU/AL бухту с положительным остатком. Выбор сохраняется отдельно как immutable spool-selection до UART delivery.

`RUN_COMPLETED` сам провод не списывает.

После завершения UI предлагает immutable выбранную бухту, но оператор вручную вводит фактический вес и подтверждает складскую операцию.

Новый write-off provenance имеет гранулярность конкретного прогона:

```text
source_session_id + source_run_id
```

Backend проверяет completed run, repair/spool identity и отсутствие предыдущего CONFIRMED для того же run. Recovery переносит provenance через `PENDING → CONFIRMED | ABORTED`.

Repair finalization требует ручное покрытие каждого нового completed linked run, для которого существует immutable spool-selection. При отсутствии движения preflight возвращает `repair_finalization_wire_writeoff_required`.

### Backup/export

Добавлен read-only whitelist backup:

```text
GET /api/backup/manifest
GET /api/backup/file?name=...
GET /api/backup/sessions
GET /api/backup/session-file?kind=...&session_id=...
```

Arbitrary filesystem paths запрещены.

Тяжёлый export блокируется во время активных winding phases через persisted activity guard.

Manifest теперь разделяет:

- `export_allowed` — можно ли безопасно выполнять export сейчас;
- `snapshot_stable` — нет ли известных pending/swap/session recovery markers.

Нестабильный snapshot разрешается только как диагностическая копия и явно помечается mobile/desktop UI.

## Последние важные коммиты

```text
b36def0e  Add run level wire writeoff provenance contract
021042aa  Add run specific winding completion audit
f178fc81  Implement run specific winding completion audit
1cf28fd0  Support run level wire writeoff provenance lookup
321f43fc  Validate run level wire writeoff provenance
1eb3fdbc  Use run level provenance in warehouse core
e033197e  Use run level provenance in manual writeoff helper
3195e6e5  Use run level provenance in warehouse writeoff API
68678269  Preserve run level writeoff provenance during recovery
1b08b1c6  Expose run level wire writeoff provenance
0dc4088d  Preserve legacy session provenance in writeoff recovery
6a1139f4  Add wire writeoff coverage audit contract
20315b1f  Implement wire writeoff coverage audit
4279fa38  Require manual wire writeoff coverage before repair closure
ff2cabe3  Expose manual wire writeoff finalization requirement
7025bf68  Keep repair finalization preflight strictly read only
3c2590d6  Reject duplicate run provenance in finalization coverage
97b2d25a  Confirm run provenance after manual wire writeoff
942657f3  Enforce unique confirmed writeoff provenance
d645dde7  Bind finalization writeoff coverage to repair and spool identity
ae9282e0  Explain wire writeoff finalization requirements on mobile
b771af85  Explain wire writeoff finalization requirements on desktop
8a990995  Show run provenance in mobile wire writeoff history
e34acc79  Show run provenance in desktop wire writeoff history
805f57f0  Expose backup snapshot stability in manifest
2d4a4b3b  Show backup snapshot stability on mobile
6e52e8f0  Show backup snapshot stability on desktop
```

## Уже закрытые архитектурные задачи — не делать повторно

- persistent `job_id/session_id` allocator;
- immutable job snapshot;
- immutable exact spool selection для новых linked jobs;
- persistent runtime-state;
- recovery/manual review;
- strict repair/motor linkage;
- authoritative motor `coil_program`;
- winding journal schema 2;
- read-only winding history с cursor pagination;
- semantic winding transition audit;
- dynamic microSD readiness;
- repair `OPEN → CLOSED` lifecycle;
- запреты mutation для CLOSED;
- warehouse `PENDING → CONFIRMED | ABORTED` recovery;
- material usage/adjustment recovery;
- strict warehouse/material/costing parsing;
- repair costing и monthly closed-repair reporting;
- finalization preflight;
- exact spool identity для linked winding;
- run-level manual wire writeoff provenance;
- duplicate provenance prevention;
- manual writeoff coverage requirement перед CLOSED;
- read-only backup/export основных persistent данных;
- backup activity guard;
- backup snapshot stability manifest.

## Следующее обязательное действие — hardware E2E

Repository review и CI не доказывают физическое поведение ESP32 + Arduino.

Минимальный happy path:

```text
1. создать/выбрать клиента
2. создать/выбрать двигатель с валидной coil_program
3. создать ремонт
4. настроить warehouse price и убедиться, что есть активная CU/AL бухта
5. открыть linked winding
6. выбрать конкретный spool_id
7. отправить job
8. получить JOB_ACK ACCEPTED
9. выполнить физический START
10. получить RUN_STARTED
11. дождаться RUN_COMPLETED
12. проверить winding history и immutable spool identity
13. открыть ручное списание
14. убедиться, что предложена именно выбранная бухта и конкретный run
15. вручную ввести фактический вес после работы
16. подтвердить write-off
17. проверить source_session_id + source_run_id в истории
18. проверить costing
19. выполнить finalization preflight
20. закрыть ремонт
21. проверить read-only archive/report
22. сделать стабильный backup manifest/export
```

Обязательные отказные сценарии:

- microSD недоступна до создания job;
- microSD теряется после boot;
- reboot после ACCEPTED до physical START;
- reboot после RUN_STARTED;
- corrupted snapshot/state/journal;
- corrupted warehouse/material log;
- dangling warehouse PENDING;
- незавершённый material pending;
- попытка второго write-off для того же `(session_id, run_id)`;
- попытка close без manual wire writeoff;
- backup во время активного winding;
- backup manifest при recovery marker.

## Следующий repo-reviewable кодовый приоритет

Если физический стенд временно недоступен, следующий код выбирать из эксплуатационной доводки, а не создавать ещё один parallel production path:

1. короткий audit всех operator-facing ошибок и HTTP semantics после новых run-level/finalization кодов;
2. backup completeness: только после точной проверки добавлять дополнительные recovery markers или export-файлы;
3. performance/rotation план для растущих NDJSON журналов — без преждевременной миграции БД;
4. `analogue / unassigned winding` проектировать только как отдельное продуктовое решение, если мастерской реально нужен такой workflow, и не ослаблять `repair ↔ motor ↔ coil_program` invariant.

## Что фиксировать при hardware E2E

При расхождении сохранить:

- HTTP status и JSON `/api/jobs`, `/api/status`, `/api/warehouse/write-offs`, `/api/repairs/finalization`, `/api/repairs/close`;
- UART событие Arduino;
- `events.ndjson`;
- `spool-selection/session-N.json`;
- соответствующий `movements.ndjson` transaction pair;
- runtime-state/snapshot session при recovery-проблеме;
- backup manifest при storage/recovery проблеме.

Не обходить fail-closed блокировки временными UI-исключениями.

## Что намеренно не делать

- auto physical START;
- auto resume после reboot;
- direct SSR control с ESP32/WEB;
- automatic wire writeoff только по `RUN_COMPLETED`;
- arbitrary-path backup API;
- миграцию NDJSON в новую БД без фактической необходимости.

## Правило переноса

Новый чат сначала читает `00_READ_FIRST.md`, `01_CURRENT_STATE.md`, этот файл и актуальные исходники. Код `cmp-protocol-v1` всегда является source of truth.
