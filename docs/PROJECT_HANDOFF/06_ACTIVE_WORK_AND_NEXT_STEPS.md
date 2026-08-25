# Активная работа и следующие шаги

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

Этот файл содержит только текущую активную очередь. Старые checkpoints — history/evidence, а не backlog.

## Stable baseline

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> stable pre-CRM
stable-2026-08-25-pre-crm-redesign -> same commit
```

Все новые изменения только в `cmp-protocol-v1`.

## Authoritative design

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

Ключевая доменная схема теперь:

```text
CLIENT -> MOTOR -> REPAIR -> AS_RECEIVED
                     -> WINDING VERSION/JOBS
                     -> MATERIAL REQUEST
                          -> WAREHOUSE ISSUE/RETURN/CORRECTION
                          -> COSTING
                     -> CASH/PAYMENTS
                     -> COMPLETED
                     -> DELIVERED
```

`СКЛАД` отвечает за физические материалы. `КАССА` отвечает за деньги. `MATERIAL REQUEST` связывает ремонт, склад и финансовую калькуляцию.

## Phase A progress

### A1. Motor winding versions — GREEN
Checkpoint 97.

### A2. Repair AS_RECEIVED snapshot — GREEN
Checkpoint 98.

### A3. Runtime/read API — GREEN
Checkpoint 99.

### A4. Repair intake pending transaction foundation — GREEN
Checkpoint 100.

Foundation:

```text
/data/workshop/repair-intake.pending.json
/data/workshop/repair-intake.pending.tmp
CM_RepairIntakePendingStore
```

Исправленный regression run после initial test-only failure: SUCCESS.

### A5. Transactional POST /api/repairs — INTEGRATED, VERIFICATION NEXT

Integrated commit:

```text
92523c7c6f4c8af8c71a63c4178a4b1e41953f19
```

`RepairIntakeCoordinator` теперь должен обеспечить:

```text
prepare pending
-> exact source winding/legacy snapshot source
-> add repair
-> append AS_RECEIVED
-> verify
-> clear pending
```

Power-loss recovery выполняется до normal create-repair operation. Не объявлять GREEN до нового connector-triggered ESP32 Build + CMP.

## Current NEXT

1. Удалить одноразовый patch workflow после его использования.
2. Добавить regression на transactional `POST /api/repairs`.
3. Запустить/проверить ESP32 Build + CMP и закрыть A5 GREEN.
4. Добавить winding-version + AS_RECEIVED + intake pending/transaction stores в backup whitelist/integrity.
5. Начать **Material Request** schema.
6. Добавить append-only request movements: `ISSUE | RETURN | CORRECTION`.
7. Определить warehouse item catalog + bounded units (`kg/g/l/ml/pcs/...`).
8. Для wire ISSUE сохранить optional exact `source_session_id + source_run_id`; `RUN_COMPLETED` ничего не списывает автоматически.
9. После Material Request foundation — delivery event/store/API.
10. Затем payment/correction store/API.

## Motor Web after Phase A foundations

- `/desktop/motor-new.html`;
- `motors.html` catalog-only в стиле Arduino archive;
- убрать inline motor creation forms, оставить ссылки;
- `motor-details.html` как рабочая карточка;
- winding versions / WORKING / STARTING / conductors / Cu-Al;
- direct send WORKING/STARTING без automatic physical START;
- links to repair/material-request history.

## Client Web

- `/desktop/client-new.html`;
- `clients.html` catalog-only;
- `/desktop/client-details.html`;
- убрать inline client form из repairs;
- motors / repairs / material requests / payments / balance / delivery dates.

## Material Request / Warehouse

Главный owner заявки: `repair_id`, плюс exact `client_id + motor_id` provenance.

Target lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

После ISSUE старую позицию не переписывать. Возврат и исправление отдельными append-only movements.

Материалы: провод, лак, клинья/палочки, изоляция, подшипники и другие warehouse items.

## Wire accounting migration

Текущий exact-spool contract действует до полной migration.

Target после migration:

```text
material_request_id
source_session_id + source_run_id for run-linked wire
material class CU/AL
actual consumed weight
manual warehouse ISSUE confirmation
```

`spool_id` может остаться optional inventory metadata. Нельзя частично удалить старый spool contract до согласованного обновления job/writeoff/costing/finalization/backup/integrity/reports/tests.

## Cash/payments

Cash отделён от склада и costing.

- material request даёт подтверждённую себестоимость материалов;
- costing формирует repair charge;
- cash хранит payment/correction/refund events;
- partial/multiple payments, debt/overpayment;
- navigation payment -> repair -> request -> warehouse and back.

## Repair lifecycle

- immutable AS_RECEIVED;
- repair CLOSED != delivered;
- append-only delivery evidence;
- долг = warning + explicit operator confirmation, не hard block.

## Safety invariants — unchanged

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts material;
- warehouse ISSUE requires explicit operator action;
- run-linked wire movement keeps exact run provenance;
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Documentation discipline

После каждого meaningful block обновлять:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md when relevant
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
00_READ_FIRST.md when read-order/entrypoint changes
```

Для крупных persistence/API блоков создавать numbered checkpoint с exact commits + CI evidence.
