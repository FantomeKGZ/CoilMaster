# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Рабочий исходный код только из `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

Production behavior подтверждён GREEN through checkpoint **160**. Checkpoint **161** реализован и находится под CI. Hardware E2E пока не требуется; продолжается repo-reviewable software optimization.

## Recent optimization checkpoints

```text
148 managed RUN_WIRE spool mutation removes redundant pre-scan
149 spool/material bridge append -> one validated bridge-log pass
150 MaterialLedger confirmUsage two-pass retained as safety boundary (NO-CHANGE)
151 append-only audit -> no safe same-ledger duplicate full scan (NO-CHANGE)
152 autonomous save -> one bounded-tail latest-event read
153 dead-helper linker/flash audit -> linker GC already removes unused helpers (NO-CHANGE)
154 autonomous task query parsed once per page
155 motor similarity candidate parsed once; each stored winding program parsed once
156 motor similarity Web handler reuses one coil_program request String
157 material adjustment/usage history optional query values fetched once
158 material adjustment optional quantity/price values fetched once
159 standard conductor recommendations reuse one warehouse availability lookup per component
160 calculator warehouse diameter lookup -> binary search over sorted fixed catalogue
161 warehouse catalogue stays sorted during scan -> binary lookup + bounded insertion, no final O(N^2) sort
```

Production commits:

```text
1e831cde072f6c6152d10b7e71cb6a1e0f2a7b0e  checkpoint 152
a2f98cb377873d88d3fd103b6dfdfbabaf28ea65  checkpoint 154
415394d162de0f1c83e433cbbea3db94833b3162  checkpoint 155
78b41d38abdf89b9e72a02eea37edcd346c9610f  checkpoint 156
491fcf965fe573f91eb29bc99f6513017f3f5b1a  checkpoint 157
0596ae9ff473503bd1d21aeda0c6c4da0f2ba0da  checkpoint 158
93d858c83b3f63932d6c2809df585a017a74a6b6  checkpoint 159
1efb3c35947c77fb79b5cc7a24f0c07c5dcab67c  checkpoint 160
317273ac74e6e67208e9a94330b615bb3ba1ba08  checkpoint 161 current production
```

## Latest verified CI evidence

```text
CMP Tests #3739    33044747417 / SUCCESS  (checkpoint 159 production)
ESP32 Build #1638  33044747467 / SUCCESS  (checkpoint 159 production)
CMP Tests #3740    33044798103 / SUCCESS  (checkpoint 159 handoff)
CMP Tests #3741    33045115444 / SUCCESS
CMP Tests #3742    33045201841 / SUCCESS  (checkpoint 160 production)
ESP32 Build #1639  33045201854 / SUCCESS  (checkpoint 160 production)
CMP Tests #3743    33045247125 / SUCCESS  (checkpoint 160 handoff HEAD a0066b8...)
```

Checkpoint 161 CI started on `317273ac...`:

```text
CMP Tests #3744    33046624931 / in_progress at first direct check
ESP32 Build #1640  33046624904 / in_progress at first direct check
```

CMP host audit remains 69 mandatory steps.

## Checkpoint 160 — GREEN

`WarehouseStore::loadKnownWireDiameters()` validates the entire spool catalogue and returns the fixed `KnownWireDiameter[]` sorted ascending by `diameterHundredthsMm`. `ConductorCalculatorWeb::availableGramsFor()` now uses bounded binary search instead of a linear scan. No new heap allocation or fixed array was added; a missing diameter still returns zero. Recommendation ranking, area/deviation calculations, warehouse totals and JSON semantics are unchanged. CMP #3742 and ESP32 #1639 directly verify the production commit; CMP #3743 verifies the handoff HEAD.

## Checkpoint 161

`WarehouseStore::loadKnownWireDiameters()` previously linearly searched the discovered diameter list for each material-matching spool, then performed a separate quadratic sort after the full spool scan. The function now maintains its fixed output array in ascending diameter order while reading: lower-bound lookup uses binary search, existing diameters aggregate in place, and new diameters are inserted with bounded in-array shifting. The final O(N^2) sort is removed. Full-file validation, monotonically increasing spool-id checks, material validation, legacy-record exclusion, ACTIVE-only quantity aggregation, overflow protection, capacity fail-closed behavior and output ordering are preserved. No unbounded RAM or whole-file buffering is introduced.

## Safety / integrity boundaries that remain intentionally unchanged

- No automatic physical START, repeat START or resume.
- Arduino owns SSR; ESP32/Web never drives SSR directly.
- `RUN_COMPLETED` is evidence only and never automatically deducts wire.
- Wire write-off remains explicit/manual and tied to exact `spool_id`, `source_session_id`, `source_run_id`.
- MaterialLedger `confirmUsage()` keeps separate pre-WAL and mutation-time authoritative `materials.ndjson` reads until a shared writer lock with equivalent crash-recovery proof exists.
- Different ledgers / different mutation phases keep separate validation passes.
- No tail-only substitute for authoritative historical integrity scans.
- No unbounded vectors or whole-file buffering for growing NDJSON.
- No automatic production-data rotation/deletion/truncation and no premature DB/index migration.

## Current NEXT

1. Confirm CMP #3744 and ESP32 Build #1640 on `317273ac...`; checkpoint 161 is not GREEN until both are directly successful.
2. Then start checkpoint **162** from current `cmp-protocol-v1` HEAD.
3. Continue only with measurable runtime/storage/flash wins; reject cosmetic helpers or refactors with likely flash/RAM cost.
4. Preserve complete authoritative validation, HTTP preflight behavior, mutation-time TOCTOU checks, exact-spool provenance and deterministic recovery.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
