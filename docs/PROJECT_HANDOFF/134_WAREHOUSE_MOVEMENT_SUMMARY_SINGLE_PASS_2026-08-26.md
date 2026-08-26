# Checkpoint 134 — Warehouse movement summary single-pass

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Изменение

`/api/warehouse/summary` больше не выполняет отдельный полный integrity-pass по `movements.ndjson`, а затем второй ручной полный parser-pass ради агрегатов.

Добавлен audited summary path в `WarehouseMovementIntegrityAudit`:

```text
WarehouseMovementSummaryTotals
WarehouseMovementDiameterTotals
checkSummary(storage, monthPrefix, totals)
```

`checkSummary()` использует тот же authoritative `WarehouseWriteOffRecordCodec`, transaction pairing и `confirmedProvenanceUnique()` что и основной movement integrity audit, одновременно накапливая per-diameter:

```text
consumedMonthGrams
consumedAllTimeGrams
```

`WarehouseStore::readMovements()` теперь только:

1. сохраняет прежнее поведение создания пустого `movements.ndjson`, если файла нет;
2. вызывает `WarehouseMovementIntegrityAudit::checkSummary(...)`;
3. переносит уже валидированные totals в `m_summary`.

Старый второй manual movement parser удалён. Отдельный `WarehouseMovementIntegrityAudit::check(m_storage)` перед summary также удалён.

## Compatibility

Стабильные `check()`, `checkRepair()` и `checkSourceRun()` продолжают делегировать через прежнюю `checkInternal(...)` сигнатуру; новый summary path использует отдельный `checkInternalWithSummary(...)`. Это сохраняет существующие costing/finalization contracts без ослабления integrity.

Global exact-run provenance uniqueness не упрощалась: bounded batch + global re-scan остаются, потому что это fail-closed доказательство уникальности без unbounded RAM.

## Additional cleanup

Одноаргументный `loadWarehousePrice(WarehousePrice&)` теперь удалён полностью — и declaration, и implementation. Public остаётся только explicit:

```text
loadWarehousePrice(price, configured)
```

## Commits

```text
2f00e77826207462c056b313828478eee8e7f961  audited summary API
92a91083b838ca5566a614a4991412b784f988c2  summary aggregation in movement audit
89bb777a36c66ff55ce36a348ddc5175adc4673d  WarehouseStore summary integration + manual parser removal
e6cd7147c9e33abbc971c672f16b1a4fd3fde79b  retired price wrapper declaration removed
8cb046f66def7b4cfc8f589626d7bb2e4ca37e71  single-pass summary/price contract
e734ad77ee232a9ed17b1118179e1ffd96666cc5  preserve stable audit delegation + final source
```

## CI history

Intermediate contract-only failures while aligning stale string contracts:

```text
CMP #3592 32967384664 / FAILURE
CMP #3593 32967451695 / FAILURE
```

These were not production compile regressions. Final authoritative evidence:

```text
ESP32 Build #1585
run 32967638219
completed / success
head e734ad77ee232a9ed17b1118179e1ffd96666cc5

CMP Protocol Tests #3594
run 32967638259
completed / success
head e734ad77ee232a9ed17b1118179e1ffd96666cc5
all mandatory host audit steps SUCCESS
```

## Safety

Не изменены:

- physical START local-only;
- no automatic repeat START/resume;
- Arduino SSR ownership;
- `RUN_COMPLETED` evidence-only;
- explicit operator RUN_WIRE writeoff;
- exact Material Request/item/spool/session/run provenance;
- deterministic historical recovery;
- no automatic production rotation/deletion/truncation.

## NEXT

1. Audit MaterialLedger/runtime for duplicate validated full-log passes before reads/mutations.
2. Prefer reuse of existing one-pass preflight evidence rather than new caches or DB migration.
3. Do not weaken integrity/provenance checks merely to reduce scans.
