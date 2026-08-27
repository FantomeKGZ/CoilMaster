# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 159; checkpoint 160 under CI

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
160 warehouse diameter lookup in calculator uses binary search over sorted catalogue
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
1efb3c35947c77fb79b5cc7a24f0c07c5dcab67c  checkpoint 160 current production HEAD
```

Latest direct verification:

```text
CMP Tests #3735    33044295465 / SUCCESS  (checkpoint 158 production)
ESP32 Build #1635  33044295425 / SUCCESS  (checkpoint 158 production)
CMP Tests #3736    33044341585 / SUCCESS  (checkpoint 158 handoff)
CMP Tests #3737    33044584477 / SUCCESS
ESP32 Build #1636  33044584489 / SUCCESS
CMP Tests #3738    33044637015 / SUCCESS
ESP32 Build #1637  33044637005 / SUCCESS
CMP Tests #3739    33044747417 / SUCCESS  (checkpoint 159 production)
ESP32 Build #1638  33044747467 / SUCCESS  (checkpoint 159 production)
CMP Tests #3740    33044798103 / SUCCESS  (checkpoint 159 handoff HEAD c64dbc4...)
```

Checkpoint 160 verification started on `1efb3c35...`:

```text
CMP Tests #3742    33045201841 / in_progress at first direct check
ESP32 Build #1639  33045201854 / in_progress at first direct check
```

CMP host audit remains 69 mandatory steps.

## Checkpoint 159 — GREEN

Standard conductor recommendations now perform one warehouse availability lookup per component and reuse that value for `warehouse_available` and `available_g`. The fixed cache is `uint32_t[MaxConversionComponents]` (maximum two entries). Ranking, target-area/deviation math and JSON semantics are unchanged. CMP #3739 + ESP32 #1638 directly verify production; CMP #3740 verifies the handoff HEAD.

## Checkpoint 160

`WarehouseStore::loadKnownWireDiameters()` authoritatively sorts `KnownWireDiameter[]` ascending by `diameterHundredthsMm` before returning. `ConductorCalculatorWeb::availableGramsFor()` therefore no longer needs an O(N) scan. It now performs bounded binary search over that already-sorted fixed array. No new heap or fixed array is introduced, missing diameters still return zero, and all calculator ranking/output behavior remains unchanged.

The NetworkWeb repeated-`arg()` candidate was reviewed but not selected for 160: it removes only two request lookups in a large mutation handler and is lower value than the catalogue search optimization. No NetworkWeb source change was made.

The larger repair-page/status candidate remains intentionally unchanged. Repair closure order is not guaranteed to follow `repair_id`, so a lockstep status stream is invalid; request-wide status caching would require unbounded retained candidates for sparse filters.

## Current active queue — checkpoint 160 verification / 161

1. Confirm CMP #3742 and ESP32 Build #1639 on `1efb3c35...`; do not call 160 GREEN until both are successful.
2. Then start checkpoint 161 from the current `cmp-protocol-v1` HEAD.
3. Continue only with measurable runtime/storage/flash wins; no cosmetic refactors that increase code/RAM.
4. Prefer fixed-memory duplicate read/parse/lookup elimination while preserving authoritative validation.
5. Keep MaterialLedger `confirmUsage()` two-pass safety boundary.
6. No tail-only replacement of authoritative historical validation.
7. No unbounded RAM, whole-file buffering, automatic data rotation/deletion/truncation, or premature DB/index migration.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff. Arduino owns SSR. ESP32/Web never controls SSR directly. `RUN_COMPLETED` is evidence only. Exact spool/session/run provenance remains mandatory. Restore remains operator-only and fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
