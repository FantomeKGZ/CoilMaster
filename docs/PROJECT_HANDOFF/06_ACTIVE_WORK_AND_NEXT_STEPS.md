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
→ CLOSED
→ read-only архив
```

`CLOSED` является реальным server-side финальным состоянием, а не только UI-меткой.

Перед новым переходом `OPEN → CLOSED` backend теперь обязательно проверяет:

- отсутствие незавершённого/recovery winding job;
- доступность обязательных persistent storage-компонентов;
- целостность warehouse movements, material usage и pricing через строгий `RepairCosting::load()`;
- согласованность wire/material/labour total и material-aware wire breakdown;
- полную читаемость winding journal через cursor query до EOF;
- semantic transition audit всего winding journal: возрастающие session/run IDs, один активный run, `RUN_STARTED → RUN_COMPLETED`, точный рост `completed_runs`.

Если finalization audit не доказан, `CLOSED` не записывается. Повторный запрос уже закрытого ремонта остаётся идемпотентным и не требует повторной доступности warehouse/material storage.

## Последние функциональные изменения

- warehouse summary и append-paths fail-closed на overflow и partial writes;
- warehouse стоимость использует тот же `NEAREST_MINOR_UNIT`, что write-off/costing;
- mobile/desktop реестр ремонтов имеет фильтры `Открытые / Закрытые / Все`;
- закрытый ремонт ведёт в `Итог ремонта`, а не в mutation workflow;
- costing mobile/desktop показывает карточку клиента/двигателя и подтверждённый winding summary;
- создан `RepairFinalizationGuard`;
- создан отдельный `WindingJournalTransitionAudit` для semantic проверки persisted событий перед закрытием.

Последние коммиты этого блока:

```text
b6e1ea589f26122e0c433309aad3aea97ba14c5c  Fail closed on warehouse aggregate and append failures
2bc6104e214062c2cbab137e74577155af06de39  Fail closed on partial warehouse movement appends
b7dedc4eabdd0abfd2a8f7540bf1e79544afe098  Align warehouse summary value rounding
d67d89b8bdcb142185937399c50d28b49a6946e8  Add mobile repair archive filter
e3df51426f33129d321be394e6eb1926e5cb3a64  Add desktop repair archive filter
abbe9d70131c64e24c9e77299273a30979e13eba  Show repair identity in mobile costing
1dab1fd072ad9fd0b98f4a9485a85cab30608757  Show repair identity in desktop costing
5b002872a967e6a4384cf70109f2aca46e19940f  Add verified winding summary to mobile repair total
e7f14fbad856b3fdb0ea6e774cb4a2d77364c0f9  Add verified winding summary to desktop repair total
11904fc362f0551c0f98233b936550d24a60a497  Add repair finalization integrity guard contract
24680ae708f6a49d4e2171feebbb81389ab08c88  Harden repair finalization aggregate checks
24b4ba0be8b695c794110a84f7b663c8f27ee279  Gate repair closure on finalization integrity
d7cefb160fbb00099d7f77bed440b673137ca28c  Preserve idempotent repair closure
1a3a2e3021305d13d5a2cc0df4a8210fc0ceb151  Verify winding journal before repair closure
adafc73064e85f9780cd6d088dbe9e53fd960a68  Add winding journal transition audit contract
2b260e39209c7c9849c221de5e5075ed80d9ef42  Implement winding journal transition audit
34fc8325ef8cd4df69b79893e482d3d813b1c0c6  Enforce winding transition audit before repair closure
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
- явные lifecycle статусы в `/api/status` и UI;
- append-only `OPEN → CLOSED` lifecycle ремонта;
- запрет linked winding, pricing и новых списаний для `CLOSED`;
- recoverable material-ledger file swap и material-adjustment crash recovery;
- recoverable warehouse spool swap и `PENDING → CONFIRMED | ABORTED` write-off recovery;
- strict warehouse/material/costing persisted parsing и reference lookups;
- archive filters и read-only итог закрытого ремонта;
- server-side repair finalization integrity gate.

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
14. закрыть ремонт
15. убедиться, что CLOSED находится в архиве и все mutation actions заблокированы
```

## Следующий repo-reviewable функциональный блок

Если физический стенд пока недоступен, не возвращаться к уже закрытому winding infrastructure. Следующий кодовый приоритет:

1. понятные operator-facing причины finalization отказа в mobile/desktop;
2. отчёт/поиск по закрытым ремонтам: период, клиент, двигатель, прибыль/убыток;
3. только после этого — analogue/unassigned-winding workflow, если он реально нужен мастерской;
4. exact spool identity в winding job проектировать только вместе с правилами ручного подтверждения фактического расхода.

## Что фиксировать при end-to-end тесте

Если возникает расхождение, сохранить:

- HTTP status и JSON `/api/jobs`, `/api/status`, `/api/repairs/close`;
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
