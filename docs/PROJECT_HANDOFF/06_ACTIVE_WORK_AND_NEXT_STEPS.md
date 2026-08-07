# Где остановились и что делать дальше

Дата обновления: 2026-08-07
Ветка: `cmp-protocol-v1`

## Точная точка продолжения

Последний функциональный блок перед обновлением handoff:

- writer-side `CM_WindingJournal` переведён на fail-closed scans;
- corruption больше не маскируется под «событие не найдено»;
- mobile и desktop главные страницы показывают явный lifecycle задания;
- linked repair из текущего задания ведёт в историю намотки;
- `RepairRegistry` и `JobLinkageResolver` динамически замечают runtime-потерю microSD;
- формы двигателя mobile/desktop валидируют и канонизируют `coil_program` перед API.

Последние функциональные коммиты перед документацией:

```text
b33ff222617cce7ed1fd41a069a5bdeb8ff323d8  Make winding journal scans fail closed
a3b59cff66d48538cc38087c65c968346c86f54a  Harden winding journal event scans
4e56ac07590be3eb44665b7a4f1f1c0fa39d5423  Show explicit winding lifecycle on mobile
0703f828163513007e2694af28467a684d0700b2  Show explicit winding lifecycle on desktop
f7a16f4a63b8bfef595af0897509b5698bd7b8e4  Add motor catalog to mobile navigation
adede9bd93eabe338f313cd04f90482caee38df2  Detect runtime repair registry storage loss
93180abfc8d4927b4282926b3213e7f7426a92be  Detect runtime linkage storage loss
90841c8dc43fde2511a5180d528829ca0cc46d55  Validate motor winding program in mobile UI
b57a898d3921ef4c0c7dbf4a17a8e32770abbe4a  Validate motor winding program in desktop UI
```

## Уже закрытые архитектурные задачи — не делать повторно

Следующее уже реализовано и не является backlog:

- persistent `job_id/session_id` allocator;
- immutable job snapshot;
- persistent runtime-state;
- recovery evaluation и manual review closure;
- strict repair/motor linkage;
- server-authoritative motor `coil_program`;
- единый `CM_WindingProgramParser`;
- `RepairRegistry` API и `/api/motors/similar`;
- winding journal schema 2 с repair/motor context;
- read-only winding history API с cursor pagination;
- mobile/desktop winding-history pages;
- runtime microSD readiness для критических persistent stores;
- явные lifecycle статусы в `/api/status` и UI.

## Следующее действие

Продолжить **с рабочего процесса ремонта**, а не с новой инфраструктуры.

Порядок:

1. Перечитать актуальные:

```text
firmware/esp32/web/mobile/repairs.html
firmware/esp32/web/desktop/repairs.html
firmware/esp32/src/CM_RepairRegistryWeb.cpp
firmware/esp32/src/main.cpp
```

2. Проверить путь:

```text
client selection
→ motor selection
→ repair creation
→ repair card
→ linked winding page
→ POST /api/jobs with repair_id + motor_id
→ physical run
→ winding history
```

3. Устранить UI-разрывы, если ремонт не сохраняет/не восстанавливает выбранные client/motor IDs или не ведёт на linked winding/history корректно.

4. Проверить, что linked winding использует readonly программу двигателя, а сервисный режим остаётся отдельным unlinked путём.

5. После функциональной связки выполнить end-to-end checklist на реальном ESP32/Arduino.

## End-to-end checklist

Минимальный ручной сценарий:

```text
создать/выбрать клиента
создать/выбрать двигатель с coil_program
создать ремонт
открыть linked winding
создать job
получить JOB_ACK ACCEPTED
нажать физический START
получить RUN_STARTED
получить RUN_COMPLETED
проверить /api/status = PROGRAM_COMPLETED
открыть историю ремонта
убедиться, что job/session/run/repair/motor совпадают
```

Отказные сценарии после основного happy path:

- снять microSD до создания job → job creation должна быть заблокирована;
- перезапуск после ACCEPTED до физического START → manual review, без auto-resume;
- перезапуск после RUN_STARTED → manual review;
- повреждённый snapshot/state/journal → fail-closed;
- неверный repair_id/motor_id → linked job не создаётся;
- turns не совпадают с motor `coil_program` → HTTP 409;
- повторное/старое событие Arduino → не создаёт новую запись/не меняет состояние неправильно.

## Что пока намеренно не делать

- автоматическое продолжение намотки после reboot;
- автоматический физический START;
- прямое управление SSR с ESP32/WEB;
- автоматическое списание провода только по `RUN_COMPLETED`;
- миграцию NDJSON в новую БД до фактической необходимости;
- Wi-Fi manager/FTP в рамках текущего winding-flow этапа.

## Правило переноса

Новый чат должен сначала прочитать `00_READ_FIRST.md`, `01_CURRENT_STATE.md`, этот файл и актуальные исходники. Документация описывает точку продолжения, но код ветки `cmp-protocol-v1` всегда имеет приоритет.
