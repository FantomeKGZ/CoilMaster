# Checkpoint 133 — Warehouse price lookup visibility

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Изменение

Public API `WarehouseStore` теперь оставляет только явную fail-closed форму чтения цены:

```text
bool loadWarehousePrice(WarehousePrice& price, bool& configured) const
```

Одноаргументный convenience wrapper:

```text
bool loadWarehousePrice(WarehousePrice& price) const
```

перенесён в `private` и больше не доступен production callers. Это предотвращает смешение двух разных состояний:

```text
false                       -> storage / integrity read failure
true + configured == false -> price intentionally not configured
```

Его маленькая implementation пока сохранена в `CM_WarehouseStore.cpp`; удалять её отдельным большим rewrite этого файла не требуется.

## Current production callers

`WarehouseWeb`, managed RUN_WIRE и `RunWireIssueCoordinator` используют только explicit `configured` form.

## Safety / compatibility

Не менялись:

- формат `price.ndjson`;
- physical START / SSR ownership;
- explicit operator RUN_WIRE mutation;
- exact spool/session/run provenance;
- historical warehouse GET/recovery;
- no automatic material deduction;
- no automatic production deletion/rotation.

## Commits

```text
fe0992219e50800e0dbf181b365d5788f76fc445  one-arg price lookup moved to private
70f987ba58becb1d43ff744bc4ceb02c9cedc0bf  public configured-form contract
```

## Verified CI

```text
ESP32 Build #1580
run 32966647349
completed / success
head fe0992219e50800e0dbf181b365d5788f76fc445

CMP Protocol Tests #3584
run 32966706823
completed / success
head 70f987ba58becb1d43ff744bc4ceb02c9cedc0bf
```

## NEXT

1. Audit `/api/warehouse/summary` for duplicate full passes over `movements.ndjson`.
2. Prefer one authoritative validated pass that can also provide summary aggregation if existing integrity semantics can be preserved exactly.
3. No automatic rotation/deletion/truncation and no premature DB migration.
