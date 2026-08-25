# Активная работа и следующие шаги

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

Этот файл содержит только текущую активную очередь. Старые checkpoints — history/evidence, а не backlog.

## Stable baseline before new work

Перед началом Web/CRM redesign зафиксирована стабильная pre-CRM точка:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
```

`main` fast-forward синхронизирован на этот commit; дополнительно создан reference branch:

```text
stable-2026-08-25-pre-crm-redesign
```

Подробности: `docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md`.

Все следующие пункты выполняются только в `cmp-protocol-v1`. `main` не использовать как source и не двигать до следующего явно согласованного stable checkpoint.

## Current phase

Stage-1 repo-only optimization закрыт. Финальный hardware acceptance начат на реальных ESP32 + Arduino Uno и уже выявил/закрыл один операторский UX defect: клавиша `B` должна позволять безопасно прервать ошибочно начатую намотку и вернуться домой без потери run evidence. Uno и ESP32 build/CMP проверки этого исправления прошли успешно.

После этого пользователь утвердил следующий большой product/Web этап: **полный redesign Workshop Web/CRM вокруг клиента, физического двигателя, версий обмотки, ремонта, оплаты и выдачи**.

Authoritative design:

```text
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
```

Hardware acceptance остаётся обязательным release gate и будет продолжен после стабилизации затрагиваемых production contracts. Его нельзя считать завершённым.

## Phase A progress

Первый schema/persistence block выполнен:

```text
firmware/esp32/src/CM_MotorWindingVersionStore.h
firmware/esp32/src/CM_MotorWindingVersionStore.cpp
Tests/Web/check_motor_winding_version_schema.js
```

Поддерживаются:

- один physical `motor_id` -> несколько winding versions;
- predecessor `previous_version_id`;
- optional `source_repair_id`;
- отдельные WORKING / STARTING programs + repeat targets;
- bounded multi-conductor contract до 4 компонентов на role;
- canonical conductor format `CU:95x1+CU:100x1`;
- append-only `/data/workshop/motor-winding-versions.ndjson`;
- legacy `motors.ndjson` не переписывается.

Evidence:

```text
a4895a6d058a56cd3041573b38d6ec808196cc99  contract
2513d5392840fc739f402ce754f9543996086bc3  persistence
863ecd52d718f2ddfc43c74ca32ec21c18c252f8  regression
c6432c95e9f39464640a1c6ea49b097934bc5612  CI wiring
CMP #3137 / run 32844995517 / SUCCESS
ESP32 Build #1446 / run 32844995460 / SUCCESS
CMP #3140 / run 32845194923 / SUCCESS
CMP #3141 / run 32845242025 / pending at this update
```

Checkpoint: `97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md`.

## Current active implementation order

### A. Schema/contracts first

1. [IN PROGRESS] Инвентаризировать текущие motor/client/repair/winding/costing/writeoff persistence + API.
2. [DONE foundation] Определить backward-compatible winding-version schema.
3. [DONE foundation] Разделить WORKING и STARTING programs/repeat targets.
4. [DONE foundation] Определить multi-conductor representation для `0.95 + 1.00`, `0.80 x 3` и т.п.
5. [NEXT] Определить immutable `as received` repair snapshot.
6. Определить append-only delivery event (`DELIVERED_TO_CLIENT`).
7. Определить append-only payment/correction store.
8. Спроектировать целостную миграцию exact-spool -> material class + actual manual weight, без частичного удаления safety checks.
9. Добавить backup/integrity contracts для новых production stores до того, как они станут release-critical.

### B. Motor Web

10. Создать `/desktop/motor-new.html`.
11. Переделать `/desktop/motors.html` по layout/UX Arduino archive.
12. Удалить встроенные motor creation forms из catalog/repair flow; оставить ссылки на `motor-new.html`.
13. Расширить `motor-details.html` до рабочей карточки двигателя.
14. Добавить winding versions, WORKING/STARTING, Cu/Al, conductors, before/after comparison.
15. Добавить прямую отправку WORKING/STARTING на станок из карточки при сохранении physical START-only invariant.

### C. Client Web

16. Создать `/desktop/client-new.html`.
17. Переделать `/desktop/clients.html` в каталог без формы создания.
18. Создать `/desktop/client-details.html`.
19. Удалить дублированное inline создание клиента из `repairs.html`; оставить ссылку.
20. Показать в карточке клиента физические двигатели, ремонты, платежи, баланс и даты выдачи.

### D. Repair lifecycle / delivery

21. Сохранять immutable motor/winding snapshot `как поступил` при новом ремонте.
22. Показывать `как поступил` vs `после перемотки`.
23. Добавить append-only событие фактической выдачи.
24. Различать `ремонт завершён` и `выдан клиенту`.
25. Выдача при долге — warning + explicit operator confirmation, но не hard block.

### E. Wire accounting simplification

26. Убрать обязательную `spool_id` из linked workflow только после согласованной backend migration.
27. Новый расход: exact run provenance + CU/AL + фактический вес + manual confirmation.
28. `RUN_COMPLETED` по-прежнему никогда ничего автоматически не списывает.
29. Обновить costing/finalization/backup/integrity/reports одновременно.
30. Существующий spool inventory UI сохранить как необязательный интерфейс.

### F. Cash/payments

31. Добавить append-only payment/correction persistence/API.
32. Создать `/desktop/cash.html`.
33. Связать платеж с `client_id + repair_id`.
34. Поддержать частичную оплату, несколько платежей, долг/переплату и агрегированный баланс клиента.
35. Интегрировать payment history в client card и repair context.

### G. Consolidation / acceptance

36. Привести desktop navigation к новой структуре.
37. Синхронизировать mobile semantics без отдельной несовместимой схемы.
38. Добавить regression tests.
39. Проверить backup/restore whitelist/integrity для новых stores.
40. ESP32 build + CMP/Web tests.
41. Продолжить/повторить полный hardware E2E с новым final flow.

## Important migration rule — spool accounting

Пользователь одобрил переход от обязательной конкретной бухты к более простому учёту материала по фактическому весу.

Но **текущий production code пока всё ещё требует exact `spool_id`**. Поэтому до завершения Phase E старый spool invariant остаётся действующим. Нельзя удалить только Web selector и оставить backend/finalization в несовместимом состоянии.

После законченной миграции новый invariant должен быть:

```text
RUN_COMPLETED never auto-deducts wire/material.
Manual consumption remains tied to exact source_session_id + source_run_id,
plus material class (CU/AL) and actual consumed weight.
```

## Safety invariants — unchanged

Никогда не ослаблять:

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts wire/material;
- cancellation/operator abort never erases immutable run/history evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Hardware acceptance status

Hardware acceptance **не закрыт**. Уже подтверждено оператором:

- обе production firmware собираются и прошиваются;
- ESP32 boots and services initialize;
- Uno boots to normal UI;
- reboot did not auto-resume the recovered ESP32 job;
- during real operation found B-exit defect, then software fix passed Uno/ESP32/CMP builds.

Оставшаяся полная hardware sequence должна быть повторена после затрагивающих contract changes, особенно после нового winding/job/writeoff flow.

## Documentation discipline — mandatory

Во время этого redesign документация обновляется вместе с кодом:

1. после каждого meaningful implementation block обновлять этот файл;
2. checkpoint 95 обновлять фактическим статусом/решениями;
3. при изменении transfer state обновлять `90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md`;
4. при изменении current entrypoint/read order обновлять `00_READ_FIRST.md`;
5. для каждого крупного persistence/API subsystem создавать новый numbered checkpoint;
6. сохранять exact commits/CI run evidence;
7. не объявлять CI/hardware GREEN без фактической проверки.

## Read first for continuation

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md
```
