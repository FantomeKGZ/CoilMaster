# CoilMaster — repair intake transaction foundation

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN / CREATE-REPAIR INTEGRATION NEXT**

Этот checkpoint фиксирует transaction/recovery foundation для обязательного `AS_RECEIVED` snapshot нового ремонта.

## Проблема

Простой порядок:

```text
append repair
append AS_RECEIVED snapshot
```

имеет power-loss window. После первого append может остаться видимый repair без immutable intake evidence.

Поэтому автоматическая snapshot-запись не подключалась до появления durable prepare/recovery marker.

## Новый pending store

```text
/data/workshop/repair-intake.pending.json
/data/workshop/repair-intake.pending.tmp
```

Добавлены:

```text
firmware/esp32/src/CM_RepairIntakePendingStore.h
firmware/esp32/src/CM_RepairIntakePendingStore.cpp
Tests/Web/check_repair_intake_pending_contract.js
```

Pending marker содержит:

```text
repair_id
client_id
motor_id
source_winding_version_id
received_at
source_kind
```

## Durability / recovery semantics

`save()`:

1. запрещает второй active pending transaction;
2. пишет candidate в `.tmp`;
3. flush/close;
4. повторно читает и проверяет exact fields;
5. только после этого rename `.tmp -> .json`.

`begin()` / recovery:

- valid committed pending marker имеет приоритет;
- stale valid temp рядом с valid main удаляется;
- если main отсутствует, fully validated temp может быть promoted как interrupted first write;
- ambiguous/invalid main+temp state fails closed;
- active pending marker не заменяется silently новым transaction.

`clear()` выполняется только после того, как higher-level intake service доказал committed repair + committed AS_RECEIVED snapshot либо доказал, что repair вообще не был создан.

## Intended create-repair transaction

Следующий integration block должен выполнить:

```text
resolve exact expected repair_id
resolve exact intake winding source
prepare durable pending marker
append repair
verify actual repair_id == prepared repair_id
append immutable AS_RECEIVED snapshot
verify snapshot
clear pending marker
return HTTP 201
```

Boot/runtime recovery при существующем marker:

```text
repair absent
  -> transaction was never committed -> clear pending

repair present + snapshot absent
  -> reconstruct exact intake snapshot from prepared source -> append -> verify -> clear

repair present + snapshot present
  -> clear stale marker

contradiction / ambiguity / corrupt evidence
  -> fail closed; block new repair creation
```

Нельзя отвечать `201 created` до завершения snapshot commit/verification.

## Commits

```text
042ff4c26bb8819366439bbdc3cb036538ea35a3  pending contract
ae2794e119de3f01114bd891121ac4259fb117c0  pending persistence/recovery
ea50c2e9da7ff2723811acaec61a7a061bf111d3  initial regression
08ae63f53fe87c2fab45e91a534f555b3832b3d8  CI wiring
b709cb0fc3abe65cc5bf49834af8e654c83cfb19  regression expectation fix
```

## Verification

Implementation commit `ae2794e...`:

```text
ESP32 Build run 32847345517 / SUCCESS
CMP run 32847345325 / SUCCESS
```

CI wiring run `32847438939` initially FAILED only because the regression expected the single-pending guard as an exact standalone source string, while implementation had the same guard inside a combined condition.

The regression was corrected without weakening the semantic assertion.

```text
CMP run 32847719930 / SUCCESS
```

Block 100 is SOFTWARE GREEN as transaction foundation.

## Not implemented yet

- pending marker is not yet wired into `POST /api/repairs`;
- recovery is not yet orchestrated against actual repair/snapshot stores;
- new CRM stores are not yet release-critical backup/restore members.

These are the immediate next tasks.

## Safety

Unchanged:

- physical START remains local-only;
- SSR remains Arduino-owned;
- RUN_COMPLETED never performs automatic material writeoff;
- exact-spool contract remains current until coordinated migration;
- no historical repair/run evidence is silently erased or rewritten.
