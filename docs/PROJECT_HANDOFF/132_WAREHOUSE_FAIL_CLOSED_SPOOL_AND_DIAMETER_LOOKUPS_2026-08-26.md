# Checkpoint 132 — fail-closed spool and diameter lookups

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Изменение

Удалены два неоднозначных convenience overload-а `WarehouseStore`:

```text
loadActiveSpoolIdentity(spoolId, identity)
loadKnownWireDiameters(wireType, items, capacity) -> uint8_t count
```

Оставлены только fail-closed формы:

```text
bool loadActiveSpoolIdentity(spoolId, identity, found)
bool loadKnownWireDiameters(wireType, items, capacity, count)
```

Это сохраняет отдельные состояния:

```text
false                  -> I/O / integrity / invalid-input failure
true + found=false     -> spool identity отсутствует
true + count=0         -> wire catalogue успешно прочитан, но подходящих диаметров нет
```

## Production callers

- linked JOB creation использует `loadActiveSpoolIdentity(..., found)`;
- managed RUN_WIRE validate/apply/confirm использует `loadActiveSpoolIdentity(..., found)`;
- `RunWireIssueCoordinator` использует `loadActiveSpoolIdentity(..., found)`;
- conductor calculator использует `loadKnownWireDiameters(..., count)`.

Ни один production caller не требует удалённые convenience формы.

## Safety / compatibility

Не изменены:

- physical START / SSR ownership;
- explicit operator RUN_WIRE mutation;
- exact spool/session/run provenance;
- warehouse historical GET/history;
- deterministic old PENDING recovery;
- movement codecs / append helpers;
- no automatic material deduction.

## Commits

```text
6f0cae00fed57c8e90b6aa977c9658de90dc3070  declarations narrowed
237dbe93c299ea025f5199266bf78df12960a002  spool identity wrapper removed
60f7a7fa6fbaddc947bb8eb65542ee525108bf32  count-only catalogue wrapper removed
7eba027f97ad03b9e37609fd6fa1e07acec257f8  fail-closed contract coverage
```

## Verified CI

```text
ESP32 Build #1579
run 32966286119
completed / success
head 60f7a7fa6fbaddc947bb8eb65542ee525108bf32

CMP Protocol Tests #3578
run 32966344439
completed / success
head 7eba027f97ad03b9e37609fd6fa1e07acec257f8
```

## NEXT

1. Narrow the unused one-argument `loadWarehousePrice(price)` convenience overload behind `private` first.
2. Keep `loadWarehousePrice(price, configured)` public and authoritative because `configured=false` differs from read failure.
3. Remove the private wrapper implementation later only through a safe exact-file rewrite; do not risk a large unrelated source rewrite merely for cleanup.
4. Continue bounded NDJSON/runtime scan optimization after API surface cleanup.
