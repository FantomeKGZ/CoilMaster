# Где остановились и что делать дальше

Дата обновления: 2026-08-07
Ветка: `cmp-protocol-v1`

## Точная точка продолжения

Рабочий UI-путь ремонта теперь замкнут:

```text
клиент
→ двигатель
→ ремонт
→ калькуляция
→ linked winding
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ история намотки
```

Последний функциональный блок:

- desktop quick-add двигателя на `repairs.html` выровнен с mobile: similarity-проверка перед созданием;
- mobile/desktop quick-add двигателя используют ту же строгую грамматику `coil_program`, что firmware;
- страницы ремонтов безопасно обрабатывают недоступные registry API и сохраняют выбранные client/motor/repair IDs;
- linked winding mobile/desktop показывает live lifecycle текущего задания;
- lifecycle различает `WAITING_ARDUINO_ACK`, `ACCEPTED_READY`, `RUNNING`, `PROGRAM_COMPLETED`, `REJECTED`, `TIMED_OUT`, `CANCELLED`;
- linked winding имеет прямой переход в историю выбранного ремонта;
- mobile/desktop costing теперь содержит прямые действия: намотка ремонта, история намотки, списание провода, списание дополнительных материалов.

Последние функциональные коммиты:

```text
e566e6e27ee6f37804a9cae9756c8fbf73843823  Align desktop repair motor creation flow
48f5dda2daac6a2abde6b4a2b532b5a03f1503b2  Harden mobile repair creation flow
44beaaf54069525f15d29522e0f06c309bf63d30  Show linked winding lifecycle on mobile
6921b110780f34f0cad062813ab80a88a07ab6f8  Show linked winding lifecycle on desktop
9e98fd3895dfa29ecb43bfa8f747197f64259612  Link mobile costing to winding flow
56a90e8c20af48d8a5f2733089f5276ce29d11c5  Link desktop costing to winding flow
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
- mobile/desktop repair → costing → linked winding → history навигация.

## Следующее обязательное действие

Не создавать новую инфраструктуру. Выполнить **реальный end-to-end прогон полного связанного задания** на ESP32 + Arduino и сверить фактические ответы с текущим контрактом.

Минимальный сценарий:

```text
1. создать/выбрать клиента
2. создать/выбрать двигатель с валидной coil_program
3. создать ремонт
4. открыть калькуляцию ремонта
5. перейти в «Намотка ремонта»
6. убедиться, что программа readonly и соответствует карточке двигателя
7. отправить linked job
8. получить JOB_ACK ACCEPTED
9. убедиться, что UI показывает ACCEPTED_READY
10. нажать физический START
11. получить RUN_STARTED
12. убедиться, что UI показывает RUNNING
13. дождаться RUN_COMPLETED
14. убедиться, что UI показывает PROGRAM_COMPLETED
15. открыть историю намотки ремонта
16. сверить job_id + session_id + run_id + repair_id + motor_id
```

## Что фиксировать при end-to-end тесте

Если возникает расхождение, сохранить:

- HTTP status и JSON ответа `/api/jobs`;
- `/api/status` до отправки, после ACK, после RUN_STARTED и после RUN_COMPLETED;
- строку/событие UART, если проблема связана с Arduino;
- соответствующую запись `/data/winding-runs/events.ndjson`;
- при recovery-проблеме — snapshot и runtime-state этой session.

Не обходить fail-closed блокировки временными UI-исключениями: сначала определить, какой persisted/API invariant нарушен.

## Отказные сценарии после happy path

После успешного полного цикла проверить:

- снять microSD до создания job → `job_creation_ready=false`, linked job не создаётся;
- снять microSD после boot → readiness должен динамически упасть;
- перезапуск после ACCEPTED до физического START → manual review, без auto-resume;
- перезапуск после RUN_STARTED → manual review;
- повреждённый snapshot/state/journal → fail-closed;
- неверный repair_id/motor_id → linked job не создаётся;
- turns не совпадают с motor `coil_program` → HTTP 409;
- повторное/старое событие Arduino → не создаёт новую запись и не двигает состояние повторно.

## После end-to-end проверки

Если happy path и отказные проверки проходят, следующий функциональный блок выбирать из фактических задач мастерской, а не из winding-infrastructure. Приоритет:

1. состояние/закрытие ремонта и отображение выполненной намотки в карточке ремонта;
2. объединение данных ремонта, калькуляции, списаний и winding history в единую карточку;
3. только после устойчивого repair lifecycle — проектирование безопасного material write-off по факту выполнения, если для него будет определена однозначная катушка/spool identity.

## Что пока намеренно не делать

- автоматическое продолжение намотки после reboot;
- автоматический физический START;
- прямое управление SSR с ESP32/WEB;
- автоматическое списание провода только по `RUN_COMPLETED`;
- миграцию NDJSON в новую БД до фактической необходимости;
- Wi-Fi manager/FTP в рамках текущего winding-flow этапа.

## Правило переноса

Новый чат должен сначала прочитать `00_READ_FIRST.md`, `01_CURRENT_STATE.md`, этот файл и актуальные исходники. Документация описывает точку продолжения, но код ветки `cmp-protocol-v1` всегда имеет приоритет.
