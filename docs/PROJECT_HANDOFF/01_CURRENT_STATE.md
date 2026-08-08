# Текущее состояние CoilMaster

Дата обновления: **2026-08-08**  
Ветка: **`cmp-protocol-v1`**  
Ориентировочная функциональная готовность: **около 90%**  
Статус: **основной production workflow ремонта, намотки, склада и закрытия собран; остаются эксплуатационная доводка и hardware E2E**

## Safety boundary

CoilMaster объединяет Arduino Uno и ESP32:

- Arduino Uno — реальное время, SSR, Hall, физический START, клавиатура/LCD/buzzer;
- ESP32 — WEB, microSD, задания, workshop registry, warehouse/materials, costing, reports и backup;
- UART — delivery и события `RUN_STARTED` / `RUN_COMPLETED`.

Не менять базовые safety-правила:

- ESP32/WEB не управляют SSR напрямую;
- физический START не запускается автоматически;
- после reboot нет auto-resume;
- `RUN_COMPLETED` не выполняет automatic wire writeoff.

## Production path

```text
клиент
→ двигатель + authoritative coil_program
→ ремонт OPEN
→ costing
→ linked winding + exact spool_id
→ immutable job snapshot + immutable spool selection
→ UART delivery
→ JOB_ACK ACCEPTED
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ winding history
→ ручной wire writeoff по фактическому весу
→ source_session_id + source_run_id provenance
→ materials / pricing
→ read-only finalization preflight
→ CLOSED
→ read-only archive / monthly report
→ read-only backup/export
```

## Намотка и persisted state

Реализованы:

- persistent `job_id/session_id` allocator;
- immutable job snapshot;
- immutable exact spool-selection для linked job;
- persisted runtime-state;
- fail-safe recovery/manual review acknowledgement;
- strict repair/motor linkage;
- server-authoritative `coil_program` + единый `CM_WindingProgramParser`;
- winding journal schema 2;
- cursor read-only history;
- semantic transition audit `RUN_STARTED → RUN_COMPLETED`;
- explicit lifecycle UI.

Session persistence:

```text
/data/winding-jobs/id-state.txt
/data/winding-jobs/id-state.bak
/data/winding-jobs/snapshots/session-<id>.json
/data/winding-jobs/spool-selection/session-<id>.json
/data/winding-jobs/state/session-<id>.json
/data/winding-runs/events.ndjson
```

Deep winding backup validation уже использует authoritative `WindingJournalQuery::validateAll()` до EOF, затем отдельно `WindingJournalTransitionAudit::validate()`. Старого cursor-pagination полного scan в `CM_WindingPersistenceIntegrityAudit` нет; cursor остаётся только в пользовательском history API.

## Exact spool и ручное списание

Новый linked job требует конкретную `ACTIVE` CU/AL бухту с положительным остатком. Выбор сохраняется immutable до UART delivery.

После `RUN_COMPLETED` UI может только предложить immutable бухту. Оператор вручную вводит фактический остаток и подтверждает write-off.

Новая provenance-гранулярность:

```text
source_session_id + source_run_id
```

Server-side доказывается:

- конкретный run имеет `RUN_COMPLETED`;
- repair и spool совпадают с immutable selection;
- второй CONFIRMED для того же `(session, run)` запрещён;
- recovery сохраняет provenance через `PENDING → CONFIRMED | ABORTED`.

Run-level HTTP contract дополнительно hardened: если `source_session_id` / `source_run_id` присутствуют, они обязаны присутствовать вместе, быть непустыми и валидными non-zero IDs. Пустые параметры больше не могут молча перейти в legacy write-off path.

Repair finalization требует ручное покрытие каждого нового completed linked run с immutable spool-selection. Legacy sessions/movements читаются совместимо.

## CLOSED и finalization

`CLOSED` — server-side финальное состояние. Перед `OPEN → CLOSED` backend проверяет:

- нет незавершённого/recovery winding job;
- costing/material/warehouse persistence цела;
- winding journal полностью читается;
- winding transition state-machine согласована;
- wire coverage для новых `(session_id, run_id)` подтверждён вручную.

Preflight read-only, реальный close независимо повторяет проверки. Повторный close уже закрытого ремонта идемпотентен.

## Warehouse / materials / costing

Реализованы и hardened:

- CU/AL + legacy UNKNOWN;
- active spool catalogue;
- recoverable spool-file swap;
- warehouse write-off `PENDING → CONFIRMED | ABORTED` + startup recovery;
- material ledger, usage, adjustments, pending/recovery и recoverable swap;
- warehouse price history;
- strict persisted parsing/reference lookups;
- `RepairCosting` с currency/formula/overflow/transaction checks;
- одинаковое округление `NEAREST_MINOR_UNIT`.

## Reports и UI

Mobile/desktop имеют:

- OPEN/CLOSED/ALL repair filters;
- finalization preflight;
- operator-facing причины блокировки;
- read-only итог ремонта;
- winding history + immutable spool audit;
- write-off history с `session / run` provenance;
- monthly closed-repair report;
- client/motor/repair search и profit/loss filters.

## Read-only backup/export

Whitelist endpoints:

```text
GET /api/backup/manifest
GET /api/backup/file?name=...
GET /api/backup/sessions
GET /api/backup/session-file?kind=...&session_id=...
```

Arbitrary filesystem paths запрещены. Тяжёлый export и deep integrity scan не выполняются при active winding.

Manifest разделяет:

- `export_allowed` — можно ли сейчас безопасно выполнять export;
- `snapshot_stability_checked` — запускался ли глубокий audit;
- `snapshot_stable` — `true/false`, либо `null`, если глубокий scan намеренно не запускался из-за machine activity;
- `snapshot_stability_reason` — первый доказанный recovery/integrity failure;
- `snapshot_stability_duration_ms` — длительность уже выполненного deep audit; `null`, если audit не запускался;
- `material_catalog_record_count`, `material_usage_record_count`, `material_adjustment_record_count` — validated material records;
- `workshop_client_record_count`, `workshop_motor_record_count`, `workshop_repair_record_count`, `repair_status_record_count`, `repair_pricing_record_count` — validated business/workshop/pricing records;
- `winding_journal_record_count` — записи authoritative winding schema scan;
- `winding_snapshot_file_count`, `winding_state_file_count`, `winding_spool_selection_file_count` — validated session persistence files;
- `warehouse_movement_record_count` — validated warehouse movement records.

Все четырнадцать Stage 0 performance/observability метрик не влияют на `snapshot_stable`/`export_allowed` и не должны добавлять отдельный full-file/directory scan ради телеметрии. Duration оборачивает уже выполняемый deep audit; material/business/winding/warehouse record counts и session file counts собираются внутри уже выполняемых authoritative validation passes. Session counts публикуются только после полного успешного `WindingSessionPersistenceIntegrityAudit`; отсутствующая session directory при успешном audit даёт `0`, partial counts при failure не публикуются.

При безопасном machine-state `snapshot_stable=true` означает успешную read-only проверку **всего backup whitelist**, а также необходимых recovery/session adjuncts.

Статический whitelist покрыт так:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
/data/winding-runs/events.ndjson
/data/warehouse/spools.ndjson
/data/warehouse/movements.ndjson
/data/warehouse/price.ndjson
/data/materials/materials.ndjson
/data/materials/usage.ndjson
/data/materials/adjustments.ndjson
/data/repairs/pricing.ndjson
/data/winding-jobs/id-state.txt
/data/winding-jobs/id-state.bak          # optional, если существует
/data/settings/conductor-calculator.ndjson
```

Дополнительно deep audit проверяет:

- material/warehouse recovery и swap markers;
- allocator: canonical main state, optional `.bak`, отсутствие `id-state.tmp`;
- conductor settings: canonical `conductor-calculator.ndjson`, отсутствие `conductor-calculator.tmp` и `.bak` recovery residue;
- workshop/pricing/material/warehouse references и арифметику;
- winding journal schema до EOF + transition semantics;
- canonical session directories;
- **содержимое всех** `snapshot/state/spool-selection` session files штатными stores/parsers;
- cross-file `session_id/job_id/repair_id/motor_id/spool` identity.

Нестабильный snapshot можно скачать как diagnostic copy, если export разрешён, но UI не называет его чистым backup.

Compile-safety audit новых backup integrity modules выполнен на уровне repository review: public audit headers включают собственные типы (`FS.h`, count APIs — explicit `stdint.h`), старые `check(storage)` контракты сохранены, metrics/count overloads добавлены совместимо. Это **не** доказательство GREEN CI до фактического build result.

HTTP/error semantics audit зафиксирован в `docs/84_BACKUP_AND_RUN_LEVEL_HTTP_SEMANTICS_AUDIT.md`. Strategy для растущих NDJSON — в `docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md`; решение — измерять и оптимизировать bounded scans/rotation, без преждевременной миграции в БД. Stage 0 observability теперь покрывает material/business/winding/warehouse record counts, winding session file counts и общую deep-audit duration без отдельного telemetry scan.

Repository review также выявил composition hotspot: `MaterialPersistenceIntegrityAudit` после собственных material scans транзитивно вызывает broad workshop/pricing dependency audits, а backup orchestration затем отдельно проверяет часть тех же domains. Это не correctness bug; Stage 1 refactor делать только после измерения влияния или отдельной correctness-причины.

## Runtime microSD safety

Критические stores динамически fail-closed при потере microSD. Corruption не должен превращаться в ложный `404`; для основных reference lookups используются tri-state `success + found` контракты.

## Главный оставшийся внешний риск

Repository review и даже зелёный CI не доказывают физический motor/UART path. Нужен реальный ESP32 + Arduino E2E:

```text
linked repair
→ exact spool selection
→ JOB_ACK
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ manual wire writeoff
→ costing
→ finalization preflight
→ CLOSED
→ stable backup
```

На стенде снять `size_bytes`, material/business/winding/warehouse record counts, `winding_snapshot_file_count`, `winding_state_file_count`, `winding_spool_selection_file_count` и `snapshot_stability_duration_ms`, чтобы дальнейшие bounded-index/rotation решения принимались по измеренным данным.

Отдельно проверить reboot/manual-review, microSD loss, corrupted persistence и UART fault scenarios.

## Намеренно не автоматизировать

- physical START;
- resume после reboot;
- SSR control с ESP32/WEB;
- wire writeoff только по `RUN_COMPLETED`;
- обход fail-closed проверок.

## Product decisions после E2E

`analogue / unassigned winding` пока не имеет production-модели. Проектировать его только как отдельный workflow, если мастерской это реально нужно, не ослабляя строгий `repair ↔ motor ↔ coil_program` path.
