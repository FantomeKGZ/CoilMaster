# Текущее состояние CoilMaster

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

Этот файл описывает текущее состояние. История/evidence — в numbered checkpoints.

## Source of truth / stable baseline

Единственная рабочая source-of-truth ветка: `cmp-protocol-v1`. `main` для исходников не использовать.

Stable pre-CRM baseline:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> этот commit
stable-2026-08-25-pre-crm-redesign -> этот commit
```

## Current phase

Активен Workshop Web/CRM redesign.

Authoritative design:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md
```

Current queue: `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

Полный двухплатный hardware acceptance ранее начат, выявил B/operator-exit defect и не завершён. После стабилизации новых CRM/material/writeoff contracts полный E2E требуется повторить.

## Approved domain model

```text
CLIENT
-> MOTOR
-> REPAIR
-> immutable AS_RECEIVED
-> WORKING / STARTING winding version/jobs
-> MATERIAL REQUEST
   -> WAREHOUSE physical movements
   -> COSTING/material valuation
-> CASH/PAYMENTS/BALANCE
-> repair completed
-> DELIVERED_TO_CLIENT
```

Разделение ответственности:

```text
СКЛАД = физический остаток, ISSUE/RETURN/CORRECTION, accounting cost
КАССА = начисления, оплаты, corrections/refunds, баланс
MATERIAL REQUEST = связка repair ↔ warehouse ↔ costing/cash
```

## Phase A — GREEN foundations

### Motor winding versions — GREEN
Checkpoint 97. Store `/data/workshop/motor-winding-versions.ndjson`.

Поддерживаются version history, WORKING/STARTING, predecessor/source repair, Cu/Al и bounded multi-conductor data.

### Repair AS_RECEIVED — GREEN
Checkpoint 98. Store `/data/workshop/repair-as-received.ndjson`.

### Runtime/read API — GREEN
Checkpoint 99.

```text
GET /api/motors/winding/latest
GET /api/motors/winding/versions
GET /api/repairs/as-received
```

### Repair intake pending transaction foundation — GREEN
Checkpoint 100.

Durable pending marker закрывает crash-window между repair append и mandatory AS_RECEIVED append.

## Current integrated work — transactional repair creation

`POST /api/repairs` уже переведён на `RepairIntakeCoordinator` commit:

```text
92523c7c6f4c8af8c71a63c4178a4b1e41953f19
```

Coordinator должен выполнить:

```text
prepare pending
-> determine exact winding/legacy intake source
-> add repair
-> append immutable AS_RECEIVED
-> verify
-> clear pending
```

Recovery запускается до normal create-repair operation. Этот integrated block ещё нельзя считать GREEN до нового normal ESP32 Build + CMP regression run.

## Material Request — newly approved next foundation

Checkpoint 101 фиксирует новую связку warehouse/cash.

Target stores:

```text
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
```

Target request lifecycle:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

Movement kinds:

```text
ISSUE | RETURN | CORRECTION
```

Материалы включают провод, лак, клинья/палочки, изоляцию, подшипники и другие warehouse items.

После ISSUE history не переписывается; возвраты/коррекции append-only.

## Wire accounting migration

Ранее утверждённый уход от mandatory spool теперь должен быть частью Material Request architecture.

Target:

```text
RUN_COMPLETED -> no automatic deduction
operator confirms warehouse ISSUE
material_request_id
exact source_session_id + source_run_id for run-linked wire
CU/AL + actual weight
```

Текущий production backend пока использует exact `spool_id`; старые проверки остаются до полной согласованной migration. `spool_id` может стать optional inventory metadata только после обновления всей цепочки.

## Cash / costing

Costing читает подтверждённые warehouse movements и рассчитывает себестоимость/charge.

Различать:

```text
cost_amount
charge_amount
```

Cash хранит financial events, а не складские движения. Поддержать partial/multiple payments, corrections/refunds, debt/overpayment, client balance.

## Motor/Client Web target

- `motor-new.html`, catalog-only `motors.html`, expanded `motor-details.html`;
- direct WORKING/STARTING JOB creation from motor card, без physical auto-start;
- `client-new.html`, catalog-only `clients.html`, `client-details.html`;
- client card links motors, repairs, material requests, payments, balance, delivery.

## Repair lifecycle target

- AS_RECEIVED immutable;
- repair CLOSED != delivered;
- append-only delivery event;
- debt warns but does not hard-block delivery after explicit confirmation.

## Safety invariants — unchanged

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts material;
- physical warehouse ISSUE requires explicit operator action;
- run-linked wire movement preserves exact run provenance;
- cancellation/operator abort never erases immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Storage rule

- no premature DB migration;
- no destructive migration of historical records;
- prefer append-only sidecar/version/event stores;
- every new release-critical store must enter backup whitelist + integrity validation;
- no automatic NDJSON cleanup/truncation.

## Documentation discipline

Update together with each meaningful implementation block:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
101_MATERIAL_REQUEST_WAREHOUSE_CASH_BRIDGE_2026-08-25.md when relevant
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
00_READ_FIRST.md when entrypoint/read order changes
```
