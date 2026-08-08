# Текущее состояние CoilMaster

Дата обновления: **2026-08-08**  
Ветка: **`cmp-protocol-v1`**  
Ориентировочная функциональная готовность: **около 90%**  
Статус: **основной production workflow ремонта, намотки, склада и закрытия уже собран; остаются эксплуатационная доводка и hardware E2E**

## Назначение и safety boundary

CoilMaster объединяет намоточный станок и мастерскую по ремонту электродвигателей:

- Arduino Uno — реальное время, SSR, датчик Холла, физический START, клавиатура, LCD, buzzer;
- ESP32 — веб-интерфейс, microSD, задания намотки, реестр мастерской, склад, материалы, калькуляция, отчёты и backup;
- UART — доставка задания и события `RUN_STARTED` / `RUN_COMPLETED`;
- mobile и desktop UI.

ESP32 и WEB **не управляют SSR напрямую**. Физический START остаётся аппаратным действием. После reboot нет автоматического resume. `RUN_COMPLETED` **не выполняет автоматическое списание провода**.

## Основной рабочий путь

```text
клиент
→ двигатель с authoritative coil_program
→ ремонт OPEN
→ калькуляция
→ linked winding job
→ обязательный exact spool_id
→ immutable job snapshot + immutable spool selection
→ UART delivery
→ JOB_ACK ACCEPTED
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ winding history
→ ручное списание провода с фактическим весом
→ provenance source_session_id + source_run_id
→ дополнительные материалы / pricing
→ read-only finalization preflight
→ CLOSED
→ read-only архив
→ месячный отчёт
```

## Намотка и persisted state

Реализованы:

- persistent `job_id/session_id` allocator;
- immutable job snapshot;
- отдельный immutable spool-selection store для linked job;
- mutable persisted runtime-state;
- fail-safe recovery и manual-review acknowledgement;
- strict repair/motor linkage;
- server-authoritative motor `coil_program` и единый `CM_WindingProgramParser`;
- journal schema 2 с `job_id`, `session_id`, `run_id`, `repair_id`, `motor_id`;
- read-only `/api/winding-history` с cursor pagination;
- semantic transition audit `RUN_STARTED → RUN_COMPLETED`;
- explicit UI lifecycle `WAITING_ARDUINO_ACK`, `ACCEPTED_READY`, `RUNNING`, `PROGRAM_COMPLETED` и terminal states.

Критические session-файлы:

```text
/data/winding-jobs/snapshots/session-<id>.json
/data/winding-jobs/spool-selection/session-<id>.json
/data/winding-jobs/state/session-<id>.json
/data/winding-runs/events.ndjson
```

## Exact spool identity и ручное списание

Новые linked jobs требуют конкретный `spool_id`. Backend повторно проверяет, что бухта:

- `ACTIVE`;
- имеет положительный остаток;
- классифицирована как `CU` или `AL`.

До отправки задания на Arduino сохраняется immutable spool-selection с `repair_id`, `motor_id`, `spool_id`, диаметром, материалом и весом на момент выбора.

После `RUN_COMPLETED` провод **не списывается автоматически**. UI только предлагает оператору ту бухту, которая была immutable выбрана для сессии. Оператор вручную вводит фактический вес после работы и подтверждает write-off.

Новые подтверждённые movements сохраняют provenance:

```text
source_session_id
source_run_id
```

Server-side проверяется:

- конкретный `(session_id, run_id)` имеет `RUN_COMPLETED`;
- immutable spool-selection относится к тому же ремонту;
- `spool_id` совпадает;
- второй CONFIRMED write-off для того же run запрещён;
- recovery сохраняет provenance через `PENDING → CONFIRMED | ABORTED`.

Legacy movements и legacy sessions остаются читаемыми по совместимым правилам.

## Реестр ремонта и CLOSED

Работают API клиентов, двигателей и ремонтов, similarity lookup, lifecycle и finalization preflight.

`CLOSED` — server-side финальное состояние. Для нового `OPEN → CLOSED` проверяются:

- отсутствие незавершённого/recovery winding job;
- целостность costing/material/warehouse persisted данных;
- полная читаемость winding history;
- semantic winding transition audit;
- для новых completed linked runs с immutable spool-selection — наличие ручного CONFIRMED write-off именно для `(session_id, run_id)`.

Если ручное списание провода отсутствует, preflight возвращает `repair_finalization_wire_writeoff_required`, а close не выполняется.

Повторный close уже `CLOSED` ремонта остаётся идемпотентным.

## Склад, материалы и costing

Реализованы и hardened:

- CU/AL и совместимый legacy UNKNOWN;
- active spool catalogue;
- recoverable spool-file swap;
- append-only warehouse write-off `PENDING → CONFIRMED | ABORTED`;
- startup crash recovery;
- material ledger, adjustments и repair usage;
- material pending/recovery и recoverable file swap;
- warehouse price history;
- strict reference lookups и persisted parsing;
- `RepairCosting` с проверкой формул, валюты, overflow и transaction integrity;
- одинаковое округление `NEAREST_MINOR_UNIT`;
- repair-level costing, margin/loss и read-only archive totals.

## Отчёты и UI

Mobile и desktop имеют:

- repair archive filters `Открытые / Закрытые / Все`;
- finalization preflight до кнопки close;
- понятные причины блокировки, включая незавершённое ручное списание провода;
- read-only итог закрытого ремонта;
- winding history с immutable spool audit;
- write-off history с отображением `session / run` provenance;
- месячные отчёты закрытых ремонтов;
- поиск по клиенту, телефону, двигателю, модели и repair ID;
- фильтр прибыль / убыток / ноль / неподтверждённые данные.

## Read-only backup/export

Добавлен whitelist-based backup API без произвольного filesystem path:

```text
GET /api/backup/manifest
GET /api/backup/file?name=...
GET /api/backup/sessions
GET /api/backup/session-file?kind=...&session_id=...
```

Экспорт включает основные workshop/winding/warehouse/material/pricing файлы и per-session snapshot/spool-selection/runtime-state.

Тяжёлый export server-side блокируется в активных фазах намотки через persisted activity guard.

Manifest отдельно сообщает:

- `export_allowed` — можно ли сейчас безопасно выполнять тяжёлое чтение;
- `snapshot_stable` — нет ли известных recovery markers (`*.pending`, material/warehouse swap temp/backup, session temp/invalid entry).

Нестабильный snapshot можно скачать как диагностическую копию, но UI явно не называет его чистым backup.

## Runtime microSD safety

Критические stores динамически fail-closed при потере microSD. Storage corruption не должен превращаться в ложный `404`/`not found`; для основных repair/material/warehouse lookup введены tri-state `success + found` контракты.

## Главный оставшийся внешний риск

Repository review и зелёный CI **не доказывают физическое поведение станка**. Обязателен реальный end-to-end прогон ESP32 + Arduino:

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
```

Отдельно проверить reboot/manual-review, снятие microSD, повреждённые persisted-файлы и UART fault scenarios.

## Намеренно не делать автоматически

- auto physical START;
- auto resume после reboot;
- прямое SSR управление с ESP32/WEB;
- automatic wire writeoff только по `RUN_COMPLETED`;
- обход fail-closed проверок ради UI convenience.

## Оставшиеся продуктовые направления

После hardware E2E и короткой эксплуатационной доводки следующий функционал выбирать по реальной потребности мастерской. `analogue / unassigned winding` пока не имеет готовой production-модели и не должен ослаблять строгий `repair ↔ motor ↔ coil_program` path.
