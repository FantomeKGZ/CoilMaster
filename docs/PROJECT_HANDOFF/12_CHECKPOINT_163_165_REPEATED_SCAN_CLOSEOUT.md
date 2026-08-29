# Checkpoints 163–165 — repeated-scan closeout

Дата: **2026-08-29**  
Рабочая ветка: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1`** — не изменён.

## Checkpoint 163 — Repair Delivery single-pass preflight — GREEN

Repair Delivery append больше не выполняет два последовательных полных прохода delivery journal только ради проверки существующего repair linkage и вычисления следующего `delivery_id`.

Authoritative mutation-time helper `prepareAppend(repairId, deliveryId, alreadyExists)` теперь за один валидирующий проход получает оба результата. Caller использует уже доказанную exact repair identity и выполняет status-only open-state lookup; mutation/recovery/TOCTOU границы не ослаблены.

Подтверждённая серия после промежуточных исправлений:

```text
ESP32 Build #1774         run 33265057626 / SUCCESS
Arduino RU LCD #198       run 33265057605 / SUCCESS
CMP Protocol Tests #4009  run 33265221419 / SUCCESS
CMP Protocol Tests #4010  run 33265276200 / SUCCESS
checkpoint-163 docs HEAD  eb22812e4cec4954a1ca2aedf730128cbb4b6742
```

Ранние `#4005–#4008`, `#1773` и `#197` относятся к промежуточным несовпадениям C++/source-text контрактов и не считаются GREEN evidence.

## Checkpoint 164 — spool/material bridge suffix uniqueness audit — GREEN

`SpoolMaterialBridgeStore::validateAll()` раньше при каждом fixed batch повторно перечитывал уже доказанный prefix `spool-material-bridges.ndjson`.

Теперь:

- outer reader последовательно валидирует journal и строгий рост `bridge_id`;
- fixed `BridgeAuditBatchSize = 24` сохраняется;
- duplicate `spool_id` внутри текущего batch проверяется в bounded RAM;
- после batch берётся `outer.position()`;
- secondary reader начинает с этого offset и сравнивает текущий batch только с ещё не доказанным suffix;
- ранее доказанный prefix больше не перечитывается;
- append/lookup/mutation/recovery semantics не изменены;
- нет persistent cache/index/DB или whole-file buffering.

Коммиты блока:

```text
d8862aef7ae3b3c4a6e3e7dbbe49c92d19babb77  suffix-scan implementation
63c70e59fee99f77c20135606c8d9911f8bfbd4e  warehouse scoped regression contract
8f37cb5268ee461e9b41a2307981d3fd45b9a565  update stale spool bridge contract
fb7aaa368ae21fe5041395f0df5eef959233920d  restore namespace close / final runtime head
```

Промежуточная диагностика:

```text
CMP #4011 run 33266038272 / FAILURE
```

CMake/build/CTest и новый warehouse scoped audit там прошли; падение было только из-за старого source-text assertion, ожидавшего прежний full-rescan implementation.

```text
ESP32 #1776 run 33266038221 / FAILURE
```

Это был реальный compile defect первоначальной полной замены файла: потеряна закрывающая `}` namespace `CM`. Исправлено отдельным коммитом `fb7aaa...`.

Финальное exact-head подтверждение:

```text
fb7aaa368ae21fe5041395f0df5eef959233920d
CMP Protocol Tests #4014  run 33266181118 / SUCCESS
ESP32 Build #1777         run 33266181104 / SUCCESS
```

## Checkpoint 165 — residual repeated-scan audit — NO-CHANGE

После 164 проверены оставшиеся наиболее вероятные growing-journal candidates.

### SpoolMaterialBridgeIntegrityAudit cross-reference batches

Каждый bounded bridge batch проверяет ссылки в `spools.ndjson` и `materials.ndjson`. Эти проходы нельзя заменить suffix-only схемой из checkpoint 164: `spool_id` и `warehouse_item_id` могут ссылаться на запись в любой части соответствующего журнала, включая уже пройденный prefix. Без нового persistent index/cache либо whole-file state удаление этих проходов ослабило бы referential integrity.

Решение: **NO-CHANGE**.

### MaterialUsageCorrectionIntegrityAudit

Fixed correction batches намеренно перечитывают `adjustments.ndjson` и `usage.ndjson`, потому что каждый batch обязан видеть предыдущие corrections того же source usage и доказывать cumulative over-correction limit, exact operation uniqueness и source provenance.

Решение: **NO-CHANGE**.

### CashPayment correction / append paths

Read/preflight totals и mutation-time `append()`/`analyzeAppendState()` остаются разными integrity phases. Фьюзить их без отдельного prevalidated mutation API означает либо дублировать validator semantics, либо убрать authoritative reread непосредственно перед append.

Решение: **NO-CHANGE**.

### Repair Intake / recovery paths

Повторные reads вокруг durable pending/append/recovery остаются intentional TOCTOU/recovery boundaries.

Решение: **NO-CHANGE**.

## Result

Checkpoint **165** закрыт как residual audit **NO-CHANGE**. После checkpoint 164 не найден следующий простой repo-reviewable repeated-scan выигрыш, который одновременно:

- сохраняет fail-closed semantics;
- не удаляет mutation/recovery/TOCTOU reread;
- не требует persistent cache/index/database;
- не требует whole-file buffering/unbounded RAM;
- не дублирует сложную integrity logic в новом helper.

Поэтому repeated-scan optimization на experiment-side следует считать временно исчерпанной до появления конкретного профилированного bottleneck или функционального дефекта.

## Safety invariants unchanged

- no automatic physical START/repeat START;
- no auto-resume after reboot;
- Arduino remains sole SSR owner;
- ESP32/Web never drives SSR directly;
- `RUN_COMPLETED` remains evidence only;
- RUN_WIRE writeoff remains explicit/manual;
- exact `spool_id + source_session_id + source_run_id` remains mandatory;
- no automatic history truncation/rotation/deletion;
- no unbounded growing-NDJSON buffering;
- no premature DB/index migration.

## NEXT

1. Continue only on `arduino-ru-lcd-experiment`; do not modify `cmp-protocol-v1` without an explicit transfer request.
2. Treat checkpoints 159–165 as closed unless a concrete regression is observed.
3. Do not continue speculative repeated-scan refactors merely to reduce file opens.
4. Next repo-reviewable work should be driven by a concrete defect, measured bottleneck, or the remaining experiment/Hall/LCD acceptance tasks.
5. Full two-board Arduino + ESP32 hardware E2E remains the final release acceptance gate.