# CoilMaster — current project entrypoint

Дата обновления: **2026-08-25**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Что читать новому AI / coding agent

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

`95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md` — authoritative design нового активного Web/CRM этапа.

`06_ACTIVE_WORK_AND_NEXT_STEPS.md` — только текущая очередь реализации.

`90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md` — authoritative transfer checkpoint.

`93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md` — закрытие предыдущего repo-only optimization этапа и hardware acceptance context.

Старые numbered checkpoints — history/evidence, а не backlog. Не продолжать старую задачу только потому, что исторический checkpoint содержит `next`/`pending`.

Перед изменением/deletion existing file обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить exact path. Не утверждать CI/build/hardware GREEN без фактического результата или явного подтверждения оператора.

## Current phase

Stage-1 optimization закрыт. Реальная двухплатная hardware acceptance была начата, выявила defect operator exit via key `B`; программный fix прошёл Uno/ESP32/CMP verification. Полный hardware acceptance всё ещё не завершён.

Текущий активный product этап — **Workshop Web/CRM redesign**:

```text
client
-> physical motor
-> repair
-> immutable as-received snapshot
-> winding versions (WORKING / STARTING)
-> winding job(s)
-> costing
-> payments / balance
-> repair completion
-> delivered-to-client event
```

## Approved Web/CRM decisions

- `motors.html` — только каталог в стиле Arduino archive;
- отдельный `/desktop/motor-new.html`;
- `motor-details.html` — основная рабочая карточка;
- один физический двигатель = один `motor_id`;
- Al -> Cu и последующие перемотки = winding versions, не новые motors;
- отдельные WORKING / STARTING programs and repeats;
- 3-phase: STARTING обычно отсутствует;
- multi-conductor data должна поддерживать `0.95 + 1.00`, `0.80 x 3` и аналоги;
- WORKING/STARTING можно отправлять на станок прямо из motor card;
- это никогда не означает automatic physical START;
- `clients.html` — только каталог;
- отдельный `/desktop/client-new.html`;
- отдельный `/desktop/client-details.html` с motors/repairs/payments/balance/delivery;
- inline forms добавления клиента/двигателя из repairs должны быть заменены ссылками;
- repair CLOSED и фактическая выдача — разные события;
- cash/payments отделяются от costing;
- payments append-only/correction-based;
- выдача при долге разрешена только после явного operator confirmation warning.

## Wire accounting migration — important

Пользователь одобрил будущее упрощение linked workflow: уйти от обязательной exact `spool_id` и учитывать ручной фактический расход по material class + weight.

Но **это пока не реализовано**. Текущий backend/finalization всё ещё использует exact spool identity. Нельзя частично удалить spool checks только из Web.

После полной миграции целевой invariant:

```text
RUN_COMPLETED never auto-deducts wire/material.
Manual wire usage remains tied to exact source_session_id + source_run_id,
plus CU/AL material class and actual consumed weight.
```

До окончания migration действует текущий exact-spool production contract.

## Safety invariants — never weaken

- physical START only physical/local;
- no automatic physical START between repeat cycles;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never performs automatic wire/material writeoff;
- cancellation/operator abort does not erase immutable run/history evidence;
- backup restore operator-only, transactional and fail-closed;
- no automatic production-data deletion/truncation.

## Documentation rule for current phase

Documentation is updated together with every meaningful implementation block.

Always keep current:

```text
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
this file
```

For major persistence/API additions create a new numbered checkpoint with exact commits, CI runs and migration status.

## NDJSON rule

No premature DB migration. No destructive historical rewrite. Prefer bounded append-only version/event stores where practical. Every new production store must enter backup whitelist/integrity validation before becoming release-critical.
