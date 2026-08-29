# Checkpoint 162 — Repair finalization known-repair proof reuse

Дата: **2026-08-29**  
Репозиторий: **`FantomeKGZ/CoilMaster`**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Рабочая ветка: **`arduino-ru-lcd-experiment`**

## Статус

Checkpoint **162 — GREEN**.

Production не изменён:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

## Что найдено

Оба Web finalization flow уже выполняли authoritative exact repair/open-state proof:

```cpp
m_registry.repairIsOpen(repairId, repairOpen)
```

После успешного proof они вызывали generic:

```cpp
RepairFinalizationGuard::check(SD, repairId)
```

а он через `RepairCosting::load()` повторно валидировал/сканировал `repairs.ndjson`.

`RepairCosting::loadKnownRepair()` уже существовал именно для read-only callers, которые ранее выполнили authoritative repair-journal validation + exact-ID proof. Поэтому duplicate scan можно убрать без нового cache/index/state.

## Реализация

Коммиты:

```text
6c3e967a5ca555f84bd5965e92ada22f8bc67bdd  perf(repair): add known repair finalization path
cda4c10c1019681698facd64e1f0f1151c2adfff  perf(repair): reuse known repair finalization proof
6e104dfebc50464c8b6fc8bb39b17a7fe4a41d42  perf(repair): reuse finalization repair proof
f61f7e17b227fd52e1e00a96c1f945ea1ac6749f  test(repair): guard known finalization proof reuse
```

`RepairFinalizationGuard` теперь имеет два явных пути:

- `check()` — generic, по-прежнему использует `RepairCosting::load()` и сам доказывает repair existence;
- `checkKnownRepair()` — только для caller с уже выполненным authoritative proof и использует `RepairCosting::loadKnownRepair()`.

В `RepairRegistryWeb` только два finalization handlers переключены на `checkKnownRepair()`:

- `handleRepairFinalization()`;
- `handleCloseRepair()`.

Оба перед этим по-прежнему обязаны успешно выполнить `m_registry.repairIsOpen(repairId, repairOpen)`.

## Safety / integrity boundaries

Не изменялись:

- generic `RepairFinalizationGuard::check()` остаётся self-validating для unrelated callers;
- `RepairCosting::loadKnownRepair()` пропускает только уже доказанный `repairs.ndjson` proof;
- warehouse movement transaction/provenance audit остаётся authoritative;
- material usage/correction integrity остаётся authoritative;
- pricing/currency/overflow/RUN_WIRE pending checks остаются;
- winding transition audit остаётся;
- exact manual wire-writeoff coverage остаётся;
- `handleCloseRepair()` после preflight по-прежнему вызывает `m_registry.closeRepair(...)`;
- этот mutation-time close reread не убирался и остаётся TOCTOU boundary;
- никаких cache/index/DB/unbounded collections/history truncation не добавлено.

## Regression contract

`Tests/Web/check_finalization_costing_single_pass.js` теперь явно фиксирует:

- generic finalization path -> `RepairCosting::load()`;
- known finalization path -> `RepairCosting::loadKnownRepair()`;
- оба Web handlers используют known path только после `repairIsOpen()`;
- ровно два Web known-path caller;
- generic Web finalization call там больше не допускается;
- mutation-time `m_registry.closeRepair(...)` обязателен;
- checkpoint 161 Warehouse provenance suffix semantics и RUN_WIRE costing guards остаются зафиксированы.

## CI evidence

Runtime commit `6e104df...`:

```text
ESP32 Build #1757         run 33257746469 / SUCCESS
Arduino RU LCD #181       run 33257746498 / SUCCESS
```

`CMP Protocol Tests #3970`, run `33257746468`, на этом runtime commit завершился FAILURE только в старом source-text assertion:

```text
Error: Missing authoritative costing load: if (!costing.load(repairId, summary))
```

CMake configure/build, CTest (`4/4`) и все остальные contract steps прошли. Это не runtime/compile failure; assertion ещё ожидал дорефакторинговую строку.

Regression commit `f61f7e1...` обновил contract под два явных proof path:

```text
CMP Protocol Tests #3971  run 33257805004 / SUCCESS
```

## NEXT

Продолжать только в `arduino-ru-lcd-experiment`.

Checkpoint 162 закрыт. Продолжать repo-reviewable repeated-scan/performance audit только там, где proof можно переиспользовать без удаления mutation/recovery/TOCTOU rereads. `MaterialUsageCorrectionIntegrityAudit` остаётся NO-CHANGE: его bounded batches обязаны видеть предыдущие corrections для cumulative over-correction.

Full Arduino + ESP32 hardware E2E остаётся отдельным финальным acceptance gate.
