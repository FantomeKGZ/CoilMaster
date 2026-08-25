# Текущее состояние CoilMaster

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

Этот файл описывает текущее состояние. История/evidence — в numbered checkpoints.

## Source of truth / stable baseline

Единственная рабочая source-of-truth ветка: `cmp-protocol-v1`. `main` для исходников не использовать.

Перед Web/CRM redesign зафиксирован stable pre-CRM commit:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
main -> этот commit
stable-2026-08-25-pre-crm-redesign -> этот commit
```

Все новые изменения идут только в `cmp-protocol-v1`.

## Current phase

Активен **Workshop Web/CRM redesign**, design checkpoint:

```text
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
```

Current queue:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

Полный двухплатный hardware acceptance ранее начат, выявил B/operator-exit defect и не считается завершённым. После стабилизации новых CRM/writeoff contracts полный E2E требуется повторить.

## Phase A — implemented

### Motor winding version foundation — SOFTWARE GREEN

Checkpoint:

```text
docs/PROJECT_HANDOFF/97_MOTOR_WINDING_VERSION_SCHEMA_2026-08-25.md
```

Store:

```text
/data/workshop/motor-winding-versions.ndjson
```

Поддержано:

- один physical `motor_id` -> versioned winding history;
- `previous_version_id` / optional `source_repair_id`;
- отдельные WORKING / STARTING programs + repeat targets;
- bounded multi-conductor data, до 4 components/role;
- Cu/Al representation вроде `CU:95x1+CU:100x1`;
- legacy `motors.ndjson` остаётся читаемым без destructive rewrite.

Verified:

```text
ESP32 Build #1446 / 32844995460 / SUCCESS
CMP #3137 / 32844995517 / SUCCESS
CMP #3140 / 32845194923 / SUCCESS
CMP #3141 / 32845242025 / SUCCESS
```

### Repair AS_RECEIVED snapshot foundation — SOFTWARE GREEN

Checkpoint:

```text
docs/PROJECT_HANDOFF/98_REPAIR_AS_RECEIVED_SNAPSHOT_2026-08-25.md
```

Store:

```text
/data/workshop/repair-as-received.ndjson
```

Snapshot captures immutable intake evidence:

```text
repair_id + client_id + motor_id
optional winding_version_id
motor identity / phases / slots
WORKING program/repeats/conductors
STARTING presence/program/repeats/conductors
captured_at / source_kind
```

Verified:

```text
ESP32 Build #1448 / 32846323653 / SUCCESS
CMP #3147 / 32846371316 / SUCCESS
CMP #3148 / 32846419170 / SUCCESS
```

### Runtime/read API — SOFTWARE GREEN

Checkpoint:

```text
docs/PROJECT_HANDOFF/99_CRM_WINDING_LOOKUP_API_2026-08-25.md
```

Read-only API:

```text
GET /api/motors/winding/latest?motor_id=...
GET /api/motors/winding/versions?motor_id=...&cursor=...&limit=...
GET /api/repairs/as-received?repair_id=...
```

Legacy fallback is explicit rather than treated as corruption.

Verified:

```text
ESP32 Build #1452 / 32846834525 / SUCCESS
CMP #3154 / 32846834557 / SUCCESS
CMP #3155 / 32846874546 / SUCCESS
CMP #3156 / 32846924439 / SUCCESS
```

CMP #3156 includes `Audit CRM winding/as-received lookup API` SUCCESS.

## Current NEXT — repair intake transaction

Automatic AS_RECEIVED write during `POST /api/repairs` is intentionally **not yet enabled**.

Reason: two independent appends (`repair` then `snapshot`) create a power-loss crash window. The next block must introduce fail-closed transaction/recovery semantics so a newly accepted repair cannot silently exist without immutable intake evidence.

After that:

1. add both new stores to backup whitelist/integrity;
2. implement delivery event contract;
3. implement payment/correction contract;
4. move to Motor Web (`motor-new`, new catalog, motor card).

## Approved target flow

```text
CLIENT
-> physical MOTOR
-> REPAIR
-> immutable AS_RECEIVED snapshot
-> WORKING / STARTING winding jobs
-> resulting winding version
-> COSTING
-> PAYMENTS / BALANCE
-> repair completion
-> DELIVERED_TO_CLIENT
```

## Wire accounting migration

Approved target later:

```text
source_session_id + source_run_id
material class CU/AL
actual consumed weight
manual confirmation
```

Но текущий backend всё ещё использует exact `spool_id`. Нельзя частично убрать spool requirement только из Web; migration должна одновременно обновить job/writeoff/costing/finalization/backup/integrity/reports/tests.

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
06_ACTIVE_WORK_AND_NEXT_STEPS.md
01_CURRENT_STATE.md
90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md when transfer state changes
00_READ_FIRST.md when entrypoint/read order changes
```
