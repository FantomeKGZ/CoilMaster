# NEXT CHAT TRANSFER — 2026-08-29

Дата: **2026-08-29**  
Репозиторий: **`FantomeKGZ/CoilMaster`**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Активная рабочая ветка: **`arduino-ru-lcd-experiment`**

## 1. Branch policy

- `main` как source **не использовать**.
- Production `cmp-protocol-v1` не изменять без отдельного прямого запроса пользователя.
- Вся следующая разработка выполняется только в `arduino-ru-lcd-experiment`.
- Перед изменением существующего файла обязательно получить его актуальное содержимое именно из `arduino-ru-lcd-experiment` и использовать текущий blob SHA.
- Для нового файла сначала проверить, что путь отсутствует.

Production остаётся:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

На момент подготовки этого handoff текущий commit рабочей ветки был:

```text
arduino-ru-lcd-experiment = aace495a75607acceaca9847485826df9dfeaa80
```

Важно: `c490667badb28501c03454e1326d6a4fb6b33168` — это SHA дерева этого commit, а не HEAD commit.

После создания данного handoff HEAD ветки будет новее; в новом чате сначала заново получить фактический HEAD.

## 2. Что читать в новом чате

Сначала прочитать:

- `/AGENTS.md`
- `docs/PROJECT_HANDOFF/00_READ_FIRST.md`
- `docs/PROJECT_HANDOFF/01_CURRENT_STATE.md`
- `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`
- `docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md`
- `docs/PROJECT_HANDOFF/08_NEXT_CHAT_TRANSFER_2026-08-29.md`
- `docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md`

## 3. Последний закрытый experiment checkpoint

Checkpoint **158 — GREEN**: RepairCostingWeb exact repair proof reuse.

Runtime:

```text
e460a32a0021b49a6d5262c316a1d9f83f5554d2
CMP Protocol Tests #3944  run 33195340340 / SUCCESS
ESP32 Build #1744         run 33195340296 / SUCCESS
Arduino RU LCD #168       run 33195340333 / SUCCESS
```

Regression contract:

```text
3f5d7f65782196491cf81934d0b3aa0276914a02
CMP Protocol Tests #3945  run 33195389757 / SUCCESS
```

Дополнительно пользователь подтвердил следующие CMP runs на рабочей ветке:

```text
#3941 33194910481 / SUCCESS
#3942 33195029377 / SUCCESS
#3943 33195080765 / SUCCESS
#3944 33195340340 / SUCCESS
#3945 33195389757 / SUCCESS
#3946 33195529567 / SUCCESS
#3947 33195573376 / SUCCESS
```

Старый `#3940 / 33194821111 / FAILURE` относится к промежуточному regression-контракту и уже перекрыт последующими успешными CMP runs. ESP32 #1744 и Arduino RU LCD #168 также подтверждены SUCCESS.

## 4. Текущий конкретный дефект — Autonomous winding -> normal motor card

Пользователь обнаружил реальный функциональный дефект, который теперь имеет приоритет над дальнейшим performance audit.

Симптом:

- на странице Autonomous Windings завершённую автономную обмотку можно назначить существующему двигателю;
- интерфейс показывает назначение локально;
- но данные этой обмотки **не появляются в обычной карточке двигателя**;
- если двигатель создаётся с Autonomous page, сам motor должен быть обычной canonical motor entity, а данные обмотки должны попасть в его обычную winding history/card.

Установленная причина:

- autonomous assign path записывает assignment/archive linkage;
- canonical `MotorWindingVersionStore` при этом не получает новую winding version;
- поэтому Autonomous UI знает `motor_id`, а обычная motor card не имеет данных новой обмотки.

Задача: сделать assignment завершённой autonomous winding также безопасной canonical projection в нормальную motor winding history.

## 5. Обязательные правила ролей обмоток

Пользователь отдельно уточнил multi-winding поведение.

Когда у двигателя объединяются две обмотки, система **не имеет права угадывать**, какая из них рабочая и какая пусковая.

Поддерживаемые canonical роли:

```text
WORKING  = рабочая обмотка
STARTING = пусковая обмотка
```

Требования:

- роль выбирается оператором явно;
- каждая роль хранит и показывает свой полный набор данных отдельно в одной обычной карточке двигателя;
- при добавлении второй роли первая должна быть полностью сохранена;
- если выбранная роль уже занята, тихая перезапись запрещена;
- нужна явная операция replacement/new-version либо выбор другой роли;
- replacement создаёт новую полную append-only canonical version, старая история не редактируется и не удаляется;
- brand-new motor может получить первую canonical version через `WORKING`;
- `STARTING` без существующей canonical `WORKING` должен fail closed — нельзя фабриковать рабочую обмотку.

### AUXILIARY

Autonomous assignment contract содержит `AUXILIARY`, но canonical motor winding history такой роли не поддерживает.

Поэтому:

- не преобразовывать `AUXILIARY` автоматически в `WORKING` или `STARTING`;
- для canonical projection этот вариант должен быть явно unsupported/fail-closed;
- добавить regression contract, чтобы это поведение не изменилось случайно.

## 6. Canonical full-version behavior

Canonical motor winding version должна оставаться полной:

- `WORKING` обязательна;
- `STARTING` опциональна.

При assignment:

### assign WORKING

- если у latest canonical version есть STARTING — сохранить её без изменений;
- заменить/создать только WORKING;
- если WORKING уже занята, без explicit replacement отказать.

### assign STARTING

- существующая WORKING обязательна и сохраняется полностью;
- добавить/заменить только STARTING;
- если STARTING уже занята, без explicit replacement отказать.

## 7. Idempotency / crash safety

Нельзя допустить дубли canonical versions при повторе HTTP/UI операции.

Использовать exact autonomous provenance как минимум:

```text
source_session_id
source_run_id
role
```

Если текущая canonical schema позволяет — сохранить также source type/comment/provenance.

Обязательные свойства:

- exact retry той же assignment не создаёт вторую canonical version;
- существующая дефектная запись типа assignment-only должна безопасно достроить отсутствующую canonical projection при повторе;
- порядок assignment/canonical writes и exact provenance check должны позволять восстановление после частичного сбоя;
- append-only history не переписывать.

## 8. Safety / integrity invariants — не менять

- никакого automatic physical START или repeat START;
- никакого auto-resume after reboot;
- Arduino остаётся единственным владельцем SSR;
- ESP32/Web никогда не управляют SSR напрямую;
- `RUN_COMPLETED` остаётся только evidence и сам по себе не списывает провод;
- RUN_WIRE writeoff остаётся явным/manual;
- exact `spool_id + source_session_id + source_run_id` обязателен;
- restore/recovery fail closed/operator controlled;
- MaterialLedger mutation-time TOCTOU/authoritative reread сохраняются;
- разные integrity domains не объединять только ради уменьшения I/O;
- никаких unbounded in-RAM scans/cache растущих NDJSON;
- никакой автоматической truncation/rotation/deletion production history;
- никакой преждевременной миграции в DB/index;
- никакой silent edit/delete append-only history.

## 9. Файлы, которые уже были определены как наиболее вероятные точки изменения

Сначала заново получить актуальные версии и blob SHA:

```text
firmware/esp32/src/CM_AutonomousWindingArchiveAssign.cpp
firmware/esp32/src/CM_AutonomousWindingWebCompleted.cpp
firmware/esp32/src/CM_AutonomousWindingArchive.h
firmware/esp32/src/CM_AutonomousWindingArchive.cpp
firmware/esp32/src/CM_AutonomousWindingWeb.h
firmware/esp32/src/CM_AutonomousWindingWeb.cpp
firmware/esp32/src/CM_MotorWindingVersionStore.h
firmware/esp32/src/CM_MotorWindingVersionStore.cpp
firmware/esp32/src/main.cpp
firmware/esp32/web/desktop/arduino-windings.html
firmware/esp32/web/shared/arduino-windings-archive.js
Tests/Web/check_motor_winding_version_schema.js
Tests/Web/check_winding_persistence_single_pass.js
```

Также найти **актуальный normal motor-winding API writer/handler** по текущему дереву ветки. Старый GitHub code-search индекс не считать источником истины.

Важно: в предыдущем чате выяснилось, что фактический текущий `CM_MotorWindingVersionStore` отличается от старых предположений. Нельзя использовать придуманный/устаревший API; нужно исходить только из текущего header/cpp и regression tests.

## 10. Предпочтительная архитектура исправления

Не дублировать projection logic в нескольких handlers.

Если это соответствует текущей архитектуре, сделать небольшой общий helper/service, который:

1. получает completed autonomous source + exact provenance;
2. проверяет canonical latest motor winding version;
3. проверяет requested role и occupancy;
4. проверяет explicit replace flag;
5. собирает полную новую canonical version с сохранением opposite role;
6. выполняет idempotency/provenance check;
7. append-ит новую canonical version только если exact projection ещё отсутствует;
8. затем/совместно обеспечивает autonomous assignment linkage так, чтобы retry восстанавливал частично завершённую операцию без дублей.

Не создавать отдельную параллельную motor-history модель для Autonomous.

## 11. Regression cases, которые нужно зафиксировать

Минимальный обязательный набор:

1. Existing motor + autonomous WORKING -> canonical version видна в normal motor card.
2. New motor + WORKING -> создаётся первая canonical version.
3. Existing WORKING + assign STARTING -> новая полная version содержит обе роли и сохраняет WORKING byte/field exact.
4. Existing STARTING + explicit replace WORKING -> STARTING сохраняется.
5. Две completed autonomous windings с явными WORKING/STARTING сосуществуют в одной normal motor card.
6. Occupied same role без explicit replacement -> reject.
7. Occupied same role с explicit replacement -> append new full version; old version unchanged.
8. STARTING-only brand-new motor -> reject/fail closed.
9. Exact retry `session_id + run_id + role` -> no duplicate canonical version.
10. Existing assignment-only defect record retry -> canonical projection repaired.
11. AUXILIARY -> canonical projection unsupported/fail-closed.
12. Exact source provenance retained.
13. No unbounded whole-NDJSON buffering/new cache.
14. Existing RUN_WIRE/material/safety contracts unchanged.

## 12. Порядок работы в новом чате

1. Получить текущий HEAD `arduino-ru-lcd-experiment`.
2. Прочитать документы из раздела 2.
3. Fetch актуальные canonical store + autonomous handlers + normal motor winding writer + UI + tests.
4. Перед каждым изменением существующего файла повторно fetch current file + blob SHA.
5. Реализовать runtime + focused regression contracts.
6. Проверить actual GitHub Actions results; не утверждать GREEN без фактического результата.
7. При GREEN обновить `01_CURRENT_STATE.md` и `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
8. Production `cmp-protocol-v1` не трогать.
9. Hardware two-board Arduino+ESP32 E2E остаётся отдельным финальным gate, промежуточный hardware test для этого repo-reviewable defect fix не требуется.

## 13. Стиль работы

- Писать по-русски и кратко.
- Не заменять реализацию длинным планом.
- Продолжать кодом/commit-ами без остановки, пока repo-reviewable блок не закрыт или не возникнет реальный внешний blocker.
- Не просить пользователя вручную проверять каждый commit.
- Не сообщать, что build/CI GREEN, пока это не подтверждено.
