# CoilMaster — Web/CRM motor, client, repair and cash redesign

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **APPROVED DESIGN / IMPLEMENTATION NOT STARTED**

Этот checkpoint фиксирует новую согласованную Web/CRM архитектуру перед началом реализации. Он является authoritative design для следующего большого блока Web/ESP32 работ.

## 1. Цель

Перестроить Workshop Web из набора отдельных форм в цельный рабочий поток:

```text
КЛИЕНТ
  -> его физические двигатели
  -> ремонты этих двигателей
  -> состояние двигателя при приёмке
  -> выполненная/новая версия обмотки
  -> калькуляция ремонта
  -> платежи / баланс
  -> завершение ремонта
  -> фактическая выдача клиенту
```

Каталоги становятся read/browse-oriented. Создание сущностей переносится на отдельные страницы. Карточки клиента и двигателя становятся основными рабочими экранами.

## 2. Каталог двигателей

`/desktop/motors.html` должен быть визуально и функционально перестроен на основе удачного layout `/desktop/arduino-windings.html`:

- компактная таблица/список;
- быстрый поиск;
- bounded cursor paging;
- фильтры;
- быстрый переход в карточку;
- без встроенной формы создания двигателя.

Основные отображаемые поля одной строки:

```text
Название двигателя
Количество фаз
Рабочая обмотка: coil program, например 40/40/30
Количество повторов рабочей обмотки
Пусковая обмотка: coil program, например 65/65
Количество повторов пусковой обмотки
Материал/провод кратко
Действия / открыть карточку
```

Для 3-фазного двигателя пусковая обмотка отсутствует и отображается как `—`.

Предпочтительные фильтры:

- название / производитель / модель;
- фазы;
- пазы;
- Cu / Al;
- наличие пусковой обмотки;
- программа витков;
- мощность при наличии данных.

## 3. Создание двигателя — отдельная страница

Создать отдельную страницу:

```text
/desktop/motor-new.html
```

`motors.html`, `repairs.html` и другие места больше не содержат встроенную большую форму двигателя. Вместо неё — ссылка/кнопка:

```text
+ Добавить двигатель
```

которая ведёт на `motor-new.html`.

Первичное создание двигателя должно требовать минимально необходимую identity/technical информацию, а не заставлять вводить всю обмотку сразу.

После успешного создания пользователь автоматически переходит в `motor-details.html?motor_id=...`, где может добавлять/редактировать обмоточные данные.

## 4. Физический двигатель и версии обмотки

Один физический двигатель должен оставаться одним `motor_id`.

Не создавать новый `motor_id` только потому, что двигатель был перемотан с Al на Cu или получил другую обмотку.

Новая модель:

```text
motor_id
  -> winding version 1: AS_RECEIVED / ORIGINAL
  -> winding version 2: REWOUND / CURRENT
  -> winding version 3: later repair
  -> ...
```

Версия обмотки должна иметь связь с предыдущей версией (`converted_from` / predecessor semantics), чтобы можно было показать цепочку:

```text
Исходная Al -> перемотана Cu -> следующая перемотка
```

Старые данные не перезаписывать так, чтобы терялась история.

## 5. Рабочая и пусковая обмотки

Каждая winding version должна поддерживать отдельные role-блоки:

```text
WORKING
STARTING
```

Для каждого role:

- `coil_program` — например `40/40/30`;
- `repeat_target`;
- coil pitch / шаг при наличии;
- wire material (`CU` / `AL`);
- один или несколько conductor specifications;
- комментарий/источник/статус доверия при необходимости.

Для 3-фазных двигателей STARTING обычно отсутствует; UI должен автоматически это учитывать.

## 6. Несколько жил / проводов

Не ограничивать обмотку моделью `один диаметр + parallel_strands`, потому что реальный случай может быть:

```text
0.95 + 1.00
0.80 x 3
0.71 x 2 + 0.80
```

Новая conductor-модель должна позволять массив/список составляющих:

```text
diameter
strand_count
material
```

или эквивалентную bounded representation.

Это также станет основой для Al -> Cu calculator/conversion history.

## 7. Карточка двигателя

`/desktop/motor-details.html?motor_id=...` становится рабочим центром двигателя.

Блоки:

1. identity / паспортные данные;
2. текущая winding version;
3. WORKING winding;
4. STARTING winding (если применимо);
5. история winding versions;
6. сравнение `как поступил` vs `после перемотки`;
7. ремонты двигателя;
8. источник/заметки;
9. быстрые действия.

Ключевые кнопки прямо в карточке:

```text
Отправить рабочую на станок
Отправить пусковую на станок
```

Эти кнопки создают/передают JOB, но **никогда не запускают физическую намотку**. Physical START остаётся только локальной физической кнопкой.

После завершения рабочей обмотки оператор может из той же карточки отправить пусковую без перехода на отдельную страницу выбора программы.

## 8. Snapshot "как поступил"

При создании ремонта необходимо сохранять immutable/read-only snapshot состояния двигателя/обмотки на момент приёмки.

Это нужно, чтобы последующая перемотка/обновление карточки не изменяла историческую картину старого ремонта.

Ремонт должен уметь показать:

```text
ПРИ ПОСТУПЛЕНИИ
- material
- WORKING program / repeats / conductor data
- STARTING program / repeats / conductor data

ПОСЛЕ РЕМОНТА
- новая winding version
- фактически применённые данные
```

## 9. Клиенты — отдельный каталог и отдельное создание

`/desktop/clients.html` становится только каталогом клиентов.

Создать:

```text
/desktop/client-new.html
/desktop/client-details.html?client_id=...
```

Встроенные формы создания клиента из `clients.html` и `repairs.html` должны быть удалены после появления нового flow. Вместо них — ссылка `+ Добавить клиента`.

Каталог клиентов должен поддерживать поиск минимум по:

- имени;
- телефону;
- ID.

## 10. Карточка клиента

`client-details.html` должна показывать:

- имя;
- телефон;
- комментарии/заметки;
- двигатели, которые клиент привозил;
- ссылки на физические motor cards;
- ремонты;
- текущие/open ремонты;
- завершённые, но ещё не выданные ремонты;
- начислено;
- оплачено;
- баланс/долг;
- историю платежей;
- даты приёмки/завершения/выдачи.

Связь клиента с двигателем не должна навсегда встраивать `client_id` в master motor identity. Источник связи — ремонты/ownership-history-equivalent linkage, потому что физический двигатель потенциально может сменить владельца.

## 11. Выдача двигателя

`CLOSED` ремонта и фактическая выдача клиенту — разные события.

Добавить отдельное append-only событие/record выдачи, например semantic:

```text
DELIVERED_TO_CLIENT
repair_id
client_id
motor_id
delivered_at
comment optional
```

Нужно уметь различать:

```text
ремонт завершён
готов к выдаче
выдан клиенту
```

Оплата не должна жёстко блокировать выдачу. При наличии долга UI должен показать предупреждение, но оператор может подтвердить выдачу в долг.

## 12. Калькуляция и касса — разные подсистемы

Существующий `costing.html` сохраняет роль расчёта:

- wire/material cost;
- additional materials;
- labour;
- total cost;
- client price;
- margin/loss;
- append-only pricing revisions.

Но `costing != cash`.

Создать отдельный cash/payment layer и Web UI, например:

```text
/desktop/cash.html
```

Касса связывает:

```text
client_id
repair_id
amount
payment timestamp
payment/correction identity
```

Должны поддерживаться:

- полная оплата;
- частичная оплата;
- несколько платежей по одному ремонту;
- остаток/долг;
- клиентский агрегированный баланс;
- append-only correction вместо silent rewrite старого платежа.

Пример таблицы кассы:

```text
Дата | Клиент | Двигатель | Ремонт | Начислено | Оплачено | Остаток | Статус
```

## 13. Упрощение учёта провода — approved direction, migration required

Пользователь утвердил направление: основной рабочий UX больше не должен требовать выбора конкретной `spool_id`, потому что фактический склад бухт пока мал и exact-spool workflow создаёт лишнюю операционную нагрузку.

Предпочтительная новая модель:

```text
material class (CU/AL)
actual consumed weight
price/cost basis
manual confirmation
```

Интерфейс бухт можно сохранить как вспомогательный/необязательный inventory interface, но linked winding flow не должен требовать бухту.

КРИТИЧЕСКИ ВАЖНО: это пока **design decision, не текущий production invariant**.

До завершения миграции текущий production код всё ещё использует exact `spool_id`. Нельзя частично удалить spool checks только из Web и оставить backend/finalization в противоречивом состоянии.

Миграция должна быть атомарно согласована через:

- job creation;
- immutable snapshot/spool selection semantics;
- writeoff API;
- costing;
- repair finalization guard;
- backup integrity;
- reports/history;
- Web;
- regression tests/docs.

Новый обязательный invariant после завершения миграции:

```text
RUN_COMPLETED never auto-deducts material.
Actual wire consumption remains explicit/manual and tied to exact run evidence
(source_session_id + source_run_id) plus material class and actual weight.
```

То есть удаляется обязательность exact spool identity, но **не** ручное подтверждение расхода и **не** provenance RUN.

## 14. Safety invariants, которые не меняются

Независимо от Web/CRM redesign:

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- RUN_COMPLETED does not automatically deduct wire/material;
- cancellation/abort does not erase immutable RUN/history evidence;
- restore remains operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## 15. Compatibility / migration rules

Implementation must be backward-compatible with existing motor/client/repair records during migration.

Rules:

1. Existing motors with legacy `coil_program + repeat_target` remain readable.
2. New UI may expose them as a synthesized legacy/current WORKING version until explicitly upgraded.
3. Existing repairs/history remain readable.
4. No destructive rewrite of `motors.ndjson` just to adopt the new model.
5. Prefer append-only sidecar/event/version stores over mutation of historical master records.
6. Backup whitelist and integrity audit must be updated for every new production store.
7. Restore must understand/validate new stores before they become release-required.
8. Mobile pages must eventually follow the same data semantics; desktop implementation may lead, but no incompatible parallel schemas.

## 16. Implementation order

### Phase A — schema/contracts first

1. Inventory current motor/client/repair/costing/writeoff/cash-related endpoints and persistence.
2. Define winding-version + role + conductor schema.
3. Define client-motor/repair snapshot semantics.
4. Define delivered-to-client event/store.
5. Define payment/cash append-only schema.
6. Define weight-only manual wire usage replacement contract and compatibility path.
7. Add integrity/backup contracts before making new stores release-critical.

### Phase B — motor Web foundation

8. Create `motor-new.html`.
9. Redesign `motors.html` using Arduino archive layout.
10. Remove embedded motor creation from catalog/repair pages; replace with links.
11. Expand `motor-details.html` for winding versions + WORKING/STARTING.
12. Add create/edit/add winding data actions from motor card.
13. Add direct WORKING/STARTING send-to-machine actions from motor card while preserving physical START invariant.

### Phase C — client Web foundation

14. Create `client-new.html`.
15. Redesign `clients.html` as catalog only.
16. Create `client-details.html`.
17. Remove duplicated inline client creation from repairs; replace with links.
18. Show linked physical motors and repair history in client card.

### Phase D — repair snapshots and delivery

19. Add immutable `as received` motor/winding snapshot linkage to new repairs.
20. Show before/after winding comparison.
21. Add delivered-to-client append-only event.
22. Add `ready but not delivered` views and client-card delivery dates.

### Phase E — wire accounting simplification

23. Replace mandatory exact-spool selection in linked JOB UX/backend with approved material+actual-weight model.
24. Preserve manual exact-run provenance (`source_session_id + source_run_id`).
25. Update costing/finalization/integrity/backup/report contracts atomically.
26. Keep existing spool inventory UI available but optional/non-blocking where appropriate.

### Phase F — cash/payments

27. Add append-only payment/correction persistence and API.
28. Build `cash.html`.
29. Link payment to client + repair.
30. Add repair/client balances and payment history.
31. Integrate delivery warning when balance remains unpaid, without hard-blocking operator delivery.

### Phase G — consolidation/acceptance

32. Update desktop navigation and corresponding mobile semantics.
33. Add/expand regression tests for all new contracts.
34. Verify backup/restore whitelist/integrity for every new data store.
35. Run ESP32 build + CMP/Web regressions.
36. Perform real-device workflow acceptance for client -> motor -> repair -> winding -> cost -> payment -> delivery.

## 17. Documentation update discipline for this redesign

During implementation, documentation is part of the change, not a final cleanup task.

After every meaningful block:

1. update this checkpoint with actual implemented status and commit/run evidence;
2. update `06_ACTIVE_WORK_AND_NEXT_STEPS.md` so it contains only current queue;
3. update `90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md` when transfer state materially changes;
4. update `00_READ_FIRST.md` when the authoritative read order/current phase changes;
5. create a numbered checkpoint for each major completed subsystem if it materially changes persistence/contracts;
6. never mark implementation/CI/hardware GREEN without actual evidence.

## 18. Start condition

Before coding Phase A, current repository state and affected files must be fetched from `cmp-protocol-v1` with current blob SHAs, exactly per project rules.

Do not start by only changing HTML. Persistence/API compatibility and wire-accounting migration must be designed/implemented coherently so Web cannot claim behavior backend does not support.
