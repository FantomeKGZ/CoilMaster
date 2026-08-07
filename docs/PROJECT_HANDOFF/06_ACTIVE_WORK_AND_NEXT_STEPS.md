# Где остановились и что делать дальше

Дата обновления: 2026-08-07
Ветка: `cmp-protocol-v1`

## Точная точка продолжения

Рабочий путь ремонта теперь замкнут до архивного состояния:

```text
клиент
→ двигатель
→ ремонт OPEN
→ калькуляция
→ linked winding
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ история намотки
→ ручные списания провода/материалов
→ итог ремонта
→ read-only finalization preflight
→ finalization audit
→ CLOSED
→ read-only архив
→ месячный отчёт по закрытым ремонтам
```

`CLOSED` является server-side финальным состоянием, а не UI-меткой.

Перед новым переходом `OPEN → CLOSED` backend проверяет:

- отсутствие незавершённого/recovery winding job;
- доступность обязательных persistent storage-компонентов;
- целостность warehouse movements, material usage и pricing через строгий `RepairCosting::load()`;
- согласованность wire/material/labour total и material-aware wire breakdown;
- полную читаемость winding journal через cursor query до EOF;
- semantic transition audit winding journal: возрастающие session/run IDs, один активный run, `RUN_STARTED → RUN_COMPLETED`, точный рост `completed_runs`.

Если finalization audit не доказан, `CLOSED` не записывается. Повторный запрос уже закрытого ремонта идемпотентен и не требует повторной доступности warehouse/material storage.

## Последние функциональные изменения

- `GET /api/repairs/finalization?repair_id=...` выполняет read-only preflight без записи статуса;
- preflight возвращает `ready_to_close`, `already_closed` и точный `reason`;
- mobile/desktop карточка OPEN ремонта не запускает массовый аудит автоматически;
- оператор явно нажимает `Проверить готовность`, после успешного preflight становится доступна кнопка `Закрыть ремонт`;
- `POST /api/repairs/close` независимо повторяет все server-side проверки и остаётся authoritative safety boundary;
- mobile/desktop отчёты по закрытым ремонтам имеют поиск по клиенту, телефону, двигателю, модели и номеру ремонта;
- строки отчёта можно фильтровать по прибыли, убытку, нулевому результату и неподтверждённой калькуляции;
- фильтры строк не влияют на месячный финансовый trust: общий итог остаётся подтверждённым только если успешно прочитаны все закрытые ремонты месяца.

Новые коммиты этого блока:

```text
ec5d9988cfef4e03b2e42b66b73e9aa886be04b5  Add repair finalization preflight handler
dfc9c79c9a097db6b3a72782814e3b2b51b0f83b  Add read only repair finalization preflight API
515f18f6eb5bfd414490c757fa76d4dc8deb573c  Add repair finalization preflight to mobile UI
24dcf7b756f0fa255263d1c4bb905d1b46a9ca6a  Add repair finalization preflight to desktop UI
8e403f9cb7eaff65d8e4431611307f5405a9c48b  Add mobile repair report search and outcome filters
4b4a2473daa22912432870db56fed410636d9ad0  Add desktop repair report search and outcome filters
```

Предыдущий reporting/finalization блок:

```text
5f2a2747a0d9dcf364491fb1e3100e6a0f5cbe08  Add detailed repair finalization outcomes
dc4a2f9eb002c9b821c82f3f9bdf272b370ecb26  Distinguish repair finalization failure domains
f6172e5bb946a83b45524aa0055027c31e9ba61f  Expose detailed repair finalization failures
1426510bba067e8349d69501abcf1f94ff980052  Explain repair finalization failures on mobile
546b19f196d12e8142ba2179595b38f84fa8d767  Fix mobile repair motor form reset
f78ccefc562be7e479729a80855a3ae98ed52005  Explain repair finalization failures on desktop
29265bfcb9c2839d0abd1722dee1f9b97b259e12  Add mobile closed repair report
2e01efe8719a541bc2ad9621d2b07ac853a1a980  Add desktop closed repair report
83f0cf3347299cfe86e60aca8aff9f152f32a5f4  Link mobile repair reports
13e71d8f44acdf45940661f1626ccb9c90da922c  Link desktop repair reports
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
- append-only `OPEN → CLOSED` lifecycle ремонта;
- запрет linked winding, pricing и новых списаний для `CLOSED`;
- recoverable material-ledger file swap и material-adjustment crash recovery;
- recoverable warehouse spool swap и `PENDING → CONFIRMED | ABORTED` write-off recovery;
- strict warehouse/material/costing persisted parsing и reference lookups;
- archive filters и read-only итог закрытого ремонта;
- server-side repair finalization integrity gate;
- operator-facing finalization diagnostics;
- read-only finalization preflight;
- read-only monthly closed-repair financial report;
- client/motor/outcome filters in closed-repair reports.

## Следующее обязательное действие

Реальный end-to-end прогон полного связанного задания на ESP32 + Arduino всё ещё обязателен для доказательства физического поведения. Repository/CI не заменяет проверку UART, physical START и фактического motor path.

Минимальный happy-path сценарий:

```text
1. создать/выбрать клиента
2. создать/выбрать двигатель с валидной coil_program
3. создать ремонт
4. задать цену/стоимость работы при необходимости
5. открыть «Намотка ремонта»
6. отправить linked job
7. получить JOB_ACK ACCEPTED
8. нажать физический START
9. получить RUN_STARTED
10. дождаться RUN_COMPLETED
11. проверить winding history
12. вручную зафиксировать фактические списания провода/материалов
13. проверить итоговую калькуляцию
14. выполнить read-only finalization preflight
15. закрыть ремонт
16. убедиться, что CLOSED находится в архиве и mutation actions заблокированы
17. убедиться, что закрытый ремонт попал в месячный отчёт и находится поиском
```

## Следующий repo-reviewable функциональный блок

Если физический стенд пока недоступен, repair lifecycle/reporting сейчас достаточно завершены. Следующий кодовый приоритет выбирать из реальных задач мастерской:

1. исследовать `analogue/unassigned winding` workflow: как фиксировать намотку, когда точная карточка двигателя ещё не определена, не ослабляя linked production path;
2. exact spool identity в winding job проектировать только вместе с правилами ручного подтверждения фактического расхода — `RUN_COMPLETED` сам по себе не должен списывать провод;
3. если analogue/unassigned workflow не нужен, перейти к эксплуатационным backup/export задачам, не меняющим motor-control safety.

## Что фиксировать при end-to-end тесте

Если возникает расхождение, сохранить:

- HTTP status и JSON `/api/jobs`, `/api/status`, `/api/repairs/finalization`, `/api/repairs/close`;
- UART событие Arduino;
- соответствующие записи `events.ndjson`, movement/material usage logs;
- snapshot/runtime-state session при recovery-проблеме.

Не обходить fail-closed блокировки UI-исключениями: сначала определить нарушенный persisted/API invariant.

## Что пока намеренно не делать

- автоматическое продолжение намотки после reboot;
- автоматический физический START;
- прямое управление SSR с ESP32/WEB;
- автоматическое списание провода только по `RUN_COMPLETED`;
- миграцию NDJSON в новую БД до фактической необходимости;
- Wi-Fi manager/FTP в рамках текущего repair-flow этапа.

## Правило переноса

Новый чат должен сначала прочитать `00_READ_FIRST.md`, `01_CURRENT_STATE.md`, этот файл и актуальные исходники. Документация описывает точку продолжения, но код ветки `cmp-protocol-v1` всегда имеет приоритет.
