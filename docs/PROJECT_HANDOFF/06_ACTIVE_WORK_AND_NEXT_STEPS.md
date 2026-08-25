# Активная работа и следующие шаги

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

Этот файл содержит только текущую активную очередь. Старые checkpoints — history/evidence, а не backlog.

## Stable baseline before CRM redesign

Зафиксирован pre-CRM stable snapshot:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> этот commit
stable-2026-08-25-pre-crm-redesign -> этот commit
```

Все новые изменения выполняются только в `cmp-protocol-v1`. `main` не использовать как source и не двигать до следующего явно согласованного stable checkpoint.

## Active design

```text
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
```

Hardware acceptance был начат, но не завершён. После contract-changing CRM/writeoff изменений полный hardware E2E требуется повторить.

## Phase A progress

### A1. Motor winding versions — SOFTWARE GREEN

Checkpoint:

```text
docs/PROJECT_HANDOFF/97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md
```

Реализовано:

- append-only `/data/workshop/motor-winding-versions.ndjson`;
- один physical `motor_id` -> несколько winding versions;
- predecessor + optional repair linkage;
- WORKING / STARTING separate program + repeats;
- до 4 conductor components на role;
- canonical representation вроде `CU:95x1+CU:100x1`;
- legacy `motors.ndjson` не переписывается.

Evidence:

```text
a4895a6d058a56cd3041573b38d6ec808196cc99  contract
2513d5392840fc739f402ce754f9543996086bc3  persistence
863ecd52d718f2ddfc43c74ca32ec21c18c252f8  regression
c6432c95e9f39464640a1c6ea49b097934bc5612  CI wiring
ESP32 Build #1446 / 32844995460 / SUCCESS
CMP #3137 / 32844995517 / SUCCESS
CMP #3140 / 32845194923 / SUCCESS
CMP #3141 / 32845242025 / SUCCESS
```

### A2. Repair AS_RECEIVED snapshot — IMPLEMENTED, final CI/build verification in progress

Checkpoint:

```text
docs/PROJECT_HANDOFF/98_REPAIR_AS_RECEIVED_SNAPSHOT_2026-08-25.md
```

Реализовано foundation:

- append-only `/data/workshop/repair-as-received.ndjson`;
- exact `repair_id + client_id + motor_id` provenance;
- optional `winding_version_id`;
- immutable copy identity + phases/slots;
- WORKING program/repeats/conductors;
- STARTING presence/program/repeats/conductors;
- fail-closed duplicate lookup by repair;
- old `repairs.ndjson` / `motors.ndjson` не переписываются.

Evidence so far:

```text
d6e4652aa3ff56d89138519c4ce2d4983b06a394  contract
568beb9c34512c936685a985c360179a0c92fb76  persistence
5bba647c7346ad03b9af028365a9b2f1753c2874  regression
3d1bb7af3ac5b0f2124604b2ad29343cae78d9b5  CI wiring
CMP #3147 / 32846371316 / SUCCESS
CMP #3148 / 32846419170 / verification in progress at this update
ESP32 Build #1448 / 32846323653 / verification in progress at this update
```

Не объявлять A2 полностью GREEN, пока оба последних runs не SUCCESS.

## Current NEXT

1. Завершить verification A2 и обновить checkpoint 98.
2. Подключить `MotorWindingVersionStore` lifecycle к ESP32 runtime.
3. Добавить read/latest/page API для winding versions.
4. Подключить `RepairAsReceivedSnapshotStore` lifecycle.
5. При create repair сохранять AS_RECEIVED snapshot fail-closed, без silent history loss.
6. Добавить read API snapshot по `repair_id`.
7. Добавить оба store в backup whitelist/integrity audit до release-critical use.
8. Затем реализовать delivery event + payment/correction contracts.
9. После Phase A перейти к `motor-new.html`, redesign `motors.html`, расширенному `motor-details.html`.

## Remaining approved order

### Motor Web

- `/desktop/motor-new.html`;
- `motors.html` как catalog-only в стиле Arduino archive;
- удалить inline motor forms и оставить ссылки;
- `motor-details.html` как рабочая карточка;
- versions / WORKING / STARTING / Cu-Al / conductors / before-after;
- отправка WORKING/STARTING на станок прямо из карточки без automatic physical START.

### Client Web

- `/desktop/client-new.html`;
- `clients.html` catalog-only;
- `/desktop/client-details.html`;
- удалить inline client form из repairs;
- motors/repairs/payments/balance/delivery dates в client card.

### Repair lifecycle

- immutable AS_RECEIVED capture;
- completed != delivered;
- append-only delivery evidence;
- debt warning + explicit operator confirmation, не hard block.

### Wire accounting migration

Текущий exact-spool contract действует до полной согласованной migration.

Target после migration:

```text
source_session_id + source_run_id
material class CU/AL
actual consumed weight
manual confirmation
```

Нельзя частично удалить `spool_id` только из Web. Одновременно обновляются job/writeoff/costing/finalization/backup/integrity/reports/tests.

### Cash/payments

- append-only payment/correction store/API;
- `/desktop/cash.html`;
- `client_id + repair_id` provenance;
- partial/multiple payments, debt/overpayment, client balance.

## Safety invariants — unchanged

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

## Documentation discipline

После каждого meaningful block своевременно обновлять:

```text
95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md (при transfer-state change)
00_READ_FIRST.md (при entrypoint/read-order change)
```

Для крупных persistence/API блоков создавать numbered checkpoint с exact commits и CI evidence.
