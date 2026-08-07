# Текущее состояние CoilMaster

Дата обновления: **2026-08-07**  
Ветка: **`cmp-protocol-v1`**  
Статус: **активная разработка, основная архитектура полного цикла намотки уже собрана**

## Назначение

CoilMaster объединяет намоточный станок и мастерскую по ремонту электродвигателей:

- Arduino Uno — реальное время, SSR, датчик Холла, физический START, клавиатура, LCD, buzzer;
- ESP32 — веб-интерфейс, microSD, задания намотки, реестр мастерской, склад, калькуляция и история;
- UART — доставка задания и события `RUN_STARTED` / `RUN_COMPLETED`;
- mobile и desktop UI.

ESP32 и WEB **не управляют SSR напрямую**. Физический запуск остаётся за Arduino и аппаратной кнопкой START.

## Что уже реализовано в полном цикле намотки

Текущий путь создания связанного или сервисного задания:

```text
HTTP/UI validation
→ strict winding program parser
→ optional repair_id + motor_id validation
→ authoritative motor coil_program lookup
→ persistent job_id/session_id allocation
→ immutable job snapshot
→ runtime-state creation
→ delivery state DELIVERING
→ UART queueJob()
→ JOB_ACK persistence
→ physical START on Arduino
→ RUN_STARTED
→ RUN_COMPLETED
→ winding journal + runtime-state persistence
→ read-only winding history API/UI
```

Реализованы:

- persistent allocator `job_id/session_id`;
- immutable snapshot задания;
- mutable runtime-state;
- fail-safe recovery после перезапуска;
- `CLOSED_AFTER_REVIEW` и `POST /api/recovery/acknowledge`;
- строгая связь linked-job с `repair_id` и `motor_id`;
- серверная сверка введённых витков с `coil_program` двигателя;
- единый `CM_WindingProgramParser`;
- journal schema 2 с `job_id`, `session_id`, `run_id`, `repair_id`, `motor_id`;
- read-only `/api/winding-history` с cursor pagination;
- mobile/desktop история конкретного ремонта;
- явные lifecycle-статусы `WAITING_ARDUINO_ACK`, `ACCEPTED_READY`, `RUNNING`, `PROGRAM_COMPLETED`, terminal errors;
- запрет автоматического queue/resume после recovery.

## Реестр мастерской

Работают API:

```text
GET/POST /api/clients
GET/POST /api/motors
GET/POST /api/repairs
GET      /api/motors/similar
```

Данные:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
```

`RepairRegistry` теперь fail-closed проверяет:

- canonical `uint32` ID;
- duplicate ID;
- ссылки repair → client/motor;
- корректность `coil_program` всех сохранённых двигателей;
- частичные/неполные записи;
- runtime-доступность `/data/workshop`.

Новые `coil_program` сохраняются канонически как `N/N/...`.

## Журнал и recovery

Критические файлы:

```text
/data/winding-jobs/id-state.txt
/data/winding-jobs/snapshots/session-<session_id>.json
/data/winding-jobs/state/session-<session_id>.json
/data/winding-runs/events.ndjson
```

Snapshot и runtime-state имеют строгий persisted parser: duplicate keys, overflow, leading zeros и неоднозначные поля блокируют recovery.

`JobStateStore::loadLatest()` fail-closed на malformed state-файле и не скрывает повреждение выбором другого файла.

Журнал fail-closed проверяет:

- единственный `event`;
- canonical numeric/null fields;
- `RUN_STARTED → RUN_COMPLETED`;
- один active run на session;
- monotonic run_id;
- последовательный completed_runs;
- неизменяемый session context;
- corruption при scan не трактуется как «событие не найдено».

Read-only history применяет совместимые строгие правила. Legacy schema 1 валидируется, но не выдаётся через новый API, потому что у неё нет immutable repair/motor context.

## Runtime microSD safety

После успешного boot readiness больше не является только cached-флагом. При потере microSD динамически перестают быть ready:

- winding journal;
- persistent ID allocator;
- snapshot store;
- job state store;
- winding history query;
- static web storage;
- RepairRegistry;
- JobLinkageResolver.

Следствие: `job_creation_ready=false` и `linked_job_creation_ready=false` до попытки начать новое задание.

## Web UI

Актуальные основные страницы:

```text
firmware/esp32/web/mobile/index.html
firmware/esp32/web/mobile/repairs.html
firmware/esp32/web/mobile/motors.html
firmware/esp32/web/mobile/winding-job.html
firmware/esp32/web/mobile/winding-history.html

firmware/esp32/web/desktop/index.html
firmware/esp32/web/desktop/repairs.html
firmware/esp32/web/desktop/motors.html
firmware/esp32/web/desktop/winding-job.html
firmware/esp32/web/desktop/winding-history.html
```

Mobile и desktop формы двигателя предварительно валидируют и канонизируют `coil_program`; backend остаётся окончательным источником истины.

## Склад, материалы и калькуляция

Сохраняются ранее реализованные подсистемы:

- CU/AL/UNKNOWN;
- склад катушек и остатки;
- списания на ремонт;
- история списаний;
- стоимость и provenance;
- дополнительные материалы;
- ремонтная калькуляция;
- conductor calculator и серверные настройки.

Не начинать эти подсистемы заново.

## Статус проверки

Пользователь 2026-08-07 подтвердил, что проверенные им коммиты до начала текущей следующей серии были зелёными. Последняя большая серия после этого включает journal/UI/runtime-readiness изменения; при переносе не считать CI автоматически проверенным только по документации — сверять фактический HEAD и результаты Actions, когда они доступны.

## Текущие ограничения и риски

- `events.ndjson` и workshop NDJSON читаются линейно; при большом объёме потребуется индекс/ротация или более эффективное хранилище.
- Нет автоматического списания провода по завершению намотки: связь с конкретной складской катушкой и идемпотентность ещё не спроектированы.
- Нет автоматического resume незавершённой намотки; это намеренное safety-ограничение.
- Полный end-to-end отказный тест на реальном станке ещё не завершён.
- Wi-Fi manager/FTP и расширенные аналоги двигателей остаются отдельными отложенными задачами.

## Текущая активная область

Следующая работа после handoff — проверить и довести рабочий процесс:

```text
клиент → двигатель → ремонт → linked winding job → physical run → winding history
```

сначала на страницах `repairs.html` mobile/desktop, затем переходить к оставшимся пользовательским разрывам и end-to-end испытаниям.
