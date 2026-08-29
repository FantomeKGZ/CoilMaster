# Checkpoint 160 — Warehouse legacy spool material lookup

Дата: **2026-08-29**  
Репозиторий: **`FantomeKGZ/CoilMaster`**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Рабочая ветка: **`arduino-ru-lcd-experiment`**

## Статус

Checkpoint **160 — GREEN**.

Production не изменён:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

## Что найдено

`WarehouseStore::appendMaterialSummaryJson()` выполняет один полный read-only проход `spools.ndjson` перед чтением `movements.ndjson`. Этот первичный проход уже проверяет обязательные spool-поля и строго возрастающий `spool_id` по всему snapshot.

Для исторических CONFIRMED movement без `wire_type` старый fallback затем заново открывал `spools.ndjson` и читал его до EOF для каждого такого movement, даже когда нужный `spool_id` был найден значительно раньше. Между первичным spool-pass и этими lookup нет mutation/recovery/TOCTOU write boundary.

## Изменение

Runtime commit:

```text
5a2207bbb3866b00dc0def307b9001500d519a5f
perf(warehouse): bound legacy spool lookup scans
```

Вторичный legacy lookup теперь использует доказанную сортировку snapshot:

- читает spool rows от начала только до requested `spool_id`;
- если текущий `spool_id < requested`, продолжает;
- если `spool_id == requested`, забирает сохранённый `wire_type`, если он есть, и завершает lookup;
- если текущий `spool_id > requested`, завершает lookup сразу;
- хвост уже полностью проверенного spool snapshot повторно не читается;
- UNKNOWN semantics для отсутствующего/неразмеченного legacy spool сохраняются.

Regression contract:

```text
2e5f8d14af22783026769821a3f17f89992d2ede
test(warehouse): guard bounded legacy spool lookup
```

`Tests/Web/check_warehouse_spool_list_cleanup.js` теперь фиксирует bounded early-stop contract и запрещает возврат старого `foundSpool` tail-scan, который существовал только для повторной проверки duplicate-id после уже выполненного полного authoritative pass.

## Что намеренно не менялось

- первичный полный spool snapshot validation;
- movement transaction pairing/integrity;
- recovery rereads;
- mutation-time authoritative rereads;
- RUN_WIRE exact provenance;
- material writeoff semantics;
- append-only history;
- RAM bounds — новый cache/vector/index не добавлялся;
- automatic truncation/rotation/deletion не добавлялась;
- DB/index migration не вводилась.

## CI evidence

Runtime commit `5a2207b...`:

```text
CMP Protocol Tests #3962  run 33255863658 / SUCCESS
ESP32 Build #1753         run 33255863696 / SUCCESS
Arduino RU LCD #177       run 33255863698 / SUCCESS
```

Regression HEAD `2e5f8d1...`:

```text
CMP Protocol Tests #3963  run 33255880724 / SUCCESS
```

## NEXT

Продолжать только в `arduino-ru-lcd-experiment`.

Следующий repo-reviewable performance шаг: искать ещё один доказанный повторный growing-journal read, где можно сохранить authoritative validation и убрать только повторное чтение. Mutation/TOCTOU/recovery границы не объединять.

Full Arduino + ESP32 hardware E2E остаётся отдельным финальным acceptance gate.
