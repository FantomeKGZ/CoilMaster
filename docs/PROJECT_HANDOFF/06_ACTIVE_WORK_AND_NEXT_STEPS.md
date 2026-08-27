# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 160; checkpoint 161 under CI

```text
148 managed RUN_WIRE removes redundant spool pre-scan
149 spool/material bridge append -> one validated bridge-log pass
150 MaterialLedger confirmUsage two-pass retained as safety boundary
151 append-only audit -> no safe same-ledger duplicate full scan
152 autonomous save -> one bounded-tail latest-event read
153 dead-helper linker audit -> NO-CHANGE; linker GC already strips them
154 autonomous task query parsed once per page
155 motor similarity candidate parsed once; each stored winding program parsed once
156 motor similarity Web handler reuses one coil_program request String
157 Material History optional query values fetched once
158 Material adjustment optional quantity/price values fetched once
159 standard conductor recommendations reuse one warehouse availability lookup per component
160 calculator warehouse-diameter lookup uses binary search over sorted catalogue
161 loadKnownWireDiameters maintains sorted catalogue during scan; removes final O(N^2) sort
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
317273ac74e6e67208e9a94330b615bb3ba1ba08  checkpoint 161 current production HEAD
```

Latest direct verification:

```text
CMP Tests #3739    33044747417 / SUCCESS  (checkpoint 159 production)
ESP32 Build #1638  33044747467 / SUCCESS  (checkpoint 159 production)
CMP Tests #3740    33044798103 / SUCCESS  (checkpoint 159 handoff)
CMP Tests #3742    33045201841 / SUCCESS  (checkpoint 160 production)
ESP32 Build #1639  33045201854 / SUCCESS  (checkpoint 160 production)
CMP Tests #3743    33045247125 / SUCCESS  (checkpoint 160 handoff HEAD a0066b8...)
```

Checkpoint 161 verification started on `317273ac...`:

```text
CMP Tests #3744    33046624931 / in_progress at first direct check
ESP32 Build #1640  33046624904 / in_progress at first direct check
```

CMP host audit remains 69 mandatory steps.

## Checkpoint 160 — GREEN

`WarehouseStore::loadKnownWireDiameters()` returns the material-specific diameter catalogue sorted ascending by `diameterHundredthsMm`. `ConductorCalculatorWeb::availableGramsFor()` now uses bounded binary search instead of a linear scan. No additional RAM is introduced, a missing diameter still maps to zero grams, and recommendation ranking / JSON semantics are unchanged. CMP #3742 + ESP32 #1639 directly verify production; CMP #3743 verifies the handoff HEAD.

## Checkpoint 161

`WarehouseStore::loadKnownWireDiameters()` previously performed a linear search through already-discovered diameters for every matching spool and then ran a separate O(N^2) final sort. It now keeps the fixed `KnownWireDiameter[]` array sorted during the authoritative spool scan: lower-bound lookup is binary, an unseen diameter is inserted at the resolved position by bounded in-array shifting, and an existing diameter is aggregated in place. The final quadratic sort is removed. Capacity, overflow checks, legacy wire-type exclusion, ACTIVE-only weight aggregation, full spool-file validation and output ordering remain unchanged. No heap collection or unbounded memory is introduced.

The NetworkWeb repeated-`arg()` candidate remains lower value and unchanged. The repair-page/status request-wide scan remains rejected because close order is not guaranteed to follow `repair_id`; caching sparse candidates would violate fixed-memory bounds.

## Current active queue — checkpoint 161 verification / 162

1. Confirm CMP #3744 and ESP32 Build #1640 on `317273ac...`; do not call 161 GREEN until both are directly successful.
2. Then start checkpoint 162 from current `cmp-protocol-v1` HEAD.
3. Continue only with measurable runtime/storage/flash wins; no cosmetic refactors with likely code/RAM growth.
4. Prefer fixed-memory duplicate read/parse/lookup elimination while preserving complete authoritative validation.
5. Keep MaterialLedger `confirmUsage()` two-pass safety boundary.
6. No tail-only replacement of authoritative historical integrity validation.
7. No unbounded RAM, whole-file buffering, automatic production-data rotation/deletion/truncation, or premature DB/index migration.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff. Arduino owns SSR. ESP32/Web never controls SSR directly. `RUN_COMPLETED` is evidence only. Exact `spool_id` + `source_session_id` + `source_run_id` provenance remains mandatory for wire write-off. Restore remains operator-only and fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
