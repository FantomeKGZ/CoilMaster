# Checkpoint 161 — Warehouse provenance suffix scan

Дата: **2026-08-29**  
Репозиторий: **`FantomeKGZ/CoilMaster`**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Рабочая ветка: **`arduino-ru-lcd-experiment`**

## Статус

Checkpoint **161 — GREEN**.

Production не изменён:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

## Что найдено

`WarehouseMovementIntegrityAudit::confirmedProvenanceUnique()` сохранял до 32 CONFIRMED provenance entries, а затем для каждого batch заново открывал `movements.ndjson` с начала файла.

Из-за этого уже проверенный префикс читался повторно, а одна и та же пара provenance records могла сравниваться в обоих направлениях. Это был чистый read-only duplicate work: перед provenance uniqueness уже выполняется полный authoritative transaction/order/schema pass `checkInternalWithSummary()`, mutation/recovery/TOCTOU boundary между этими проходами отсутствует.

## Изменение

Runtime commit:

```text
dc9415c531d8c9685bc6202941df042ec299af0c
perf(warehouse): avoid duplicate provenance comparisons
```

Новый bounded алгоритм:

- размер batch остаётся фиксированным `32`;
- provenance conflicts внутри текущего batch проверяются попарно в RAM;
- после формирования batch сохраняется `outer.position()`;
- secondary scan открывается с этого byte offset и проверяет batch только против более позднего suffix;
- каждая unordered provenance pair проверяется ровно один раз;
- earlier validated prefix повторно не читается для этого batch;
- все later records по-прежнему парсятся через `WarehouseWriteOffRecordCodec::parse()` и участвуют в conflict detection;
- при невозможности seek/offset conversion audit остаётся fail-closed.

Regression contract:

```text
875c6a069b3680569dc35576d82861d737444144
test(warehouse): guard suffix-only provenance audit
```

`Tests/Web/check_finalization_costing_single_pass.js` теперь фиксирует:

- within-batch pair validation;
- `outer.position()` suffix boundary;
- обязательный seek secondary scan к suffix;
- сохранение существующего finalization/costing/RUN_WIRE integrity contract.

## Что намеренно не менялось

- первичный полный `movements.ndjson` transaction/schema/order validation;
- PENDING → final transaction pairing;
- exact RUN_WIRE `source_session_id + source_run_id + spool_id` semantics;
- duplicate provenance conflict rules, включая legacy session-level ambiguity;
- mutation-time authoritative rereads;
- recovery rereads;
- append-only movement history;
- fixed RAM bounds (`BatchSize=32`);
- automatic rotation/truncation/deletion;
- persistent cache/index/database.

## CI evidence

Runtime commit `dc9415c...`:

```text
CMP Protocol Tests #3965  run 33257271690 / SUCCESS
ESP32 Build #1754         run 33257271722 / SUCCESS
Arduino RU LCD #178       run 33257271706 / SUCCESS
```

Regression HEAD `875c6a0...`:

```text
CMP Protocol Tests #3966  run 33257294547 / SUCCESS
```

## Adjacent audit

Repair finalization remains a possible exact-proof → generic-costing candidate: Web handlers first call authoritative `RepairRegistry::repairIsOpen()` and `RepairFinalizationGuard::check()` currently uses generic `RepairCosting::load()`.

No change was made there in checkpoint 161. Removing that duplicate safely requires an explicit known-repair finalization API and caller proof; changing global `RepairFinalizationGuard::check()` to bypass repair validation would weaken unrelated callers. `closeRepair()` mutation-time authoritative reread must remain untouched.

## NEXT

Продолжать только в `arduino-ru-lcd-experiment`.

Искать следующий доказанный repeated growing-journal read с маленькой proof-preserving оптимизацией. Не объединять mutation/recovery/TOCTOU границы и не вводить persistent index/cache/DB.

Full Arduino + ESP32 hardware E2E остаётся отдельным финальным acceptance gate.
