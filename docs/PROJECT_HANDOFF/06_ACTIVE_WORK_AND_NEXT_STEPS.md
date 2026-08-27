# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 162; checkpoint 163 next

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
162 conductor recommendation search reuses precomputed single-wire areas across strand combinations
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
317273ac74e6e67208e9a94330b615bb3ba1ba08  checkpoint 161
18d611e6ee8bb0355deda5f99874b0b9923d576f  checkpoint 162
```

Latest direct verification:

```text
CMP Tests #3744    33046624931 / SUCCESS  (checkpoint 161 production)
ESP32 Build #1640  33046624904 / SUCCESS  (checkpoint 161 production)
CMP Tests #3748    33046797803 / SUCCESS  (warehouse marker-buffer optimization)
ESP32 Build #1641  33046797864 / SUCCESS  (warehouse marker-buffer optimization)
CMP Tests #3750    33046851201 / SUCCESS  (checkpoint 162 production)
ESP32 Build #1643  33046851156 / SUCCESS  (checkpoint 162 production)
```

Intermediate ESP32 Build #1642 (`33046801931`) failed on `7936b8f9964294cf0164a8e5287cfbfdc19f8c7d` because the expanded `evaluateOption()` declaration expected 10 arguments while two call sites and the definition still used 8. The corrected checkpoint-162 production commit is `18d611e6ee8bb0355deda5f99874b0b9923d576f`, verified by CMP #3750 + ESP32 #1643.

CMP host audit remains 69 mandatory steps.

## Checkpoint 162 — GREEN

`ConductorCalculator::findRecommendedOptionsForArea()` reuses precomputed single-wire areas across strand-count combinations. The first candidate area is computed once per outer candidate, the second once per pair, and the same exact integer values are passed into `evaluateOption()`. Combined area, deviation, ranking penalties, availability, component data and recommendation ordering remain unchanged. No heap allocation or unbounded memory is introduced.

The NetworkWeb repeated-`arg()` candidate remains lower value and unchanged. The repair-page/status request-wide scan remains rejected because close order is not guaranteed to follow `repair_id`; caching sparse candidates would violate fixed-memory bounds.

## Current active queue — checkpoint 163

1. Start checkpoint 163 from current `cmp-protocol-v1` HEAD.
2. Continue only with measurable runtime/storage/flash wins; no cosmetic refactors with likely code/RAM growth.
3. Prefer fixed-memory duplicate read/parse/lookup/calculation elimination while preserving complete authoritative validation.
4. Keep MaterialLedger `confirmUsage()` two-pass safety boundary.
5. No tail-only replacement of authoritative historical integrity validation.
6. No unbounded RAM, whole-file buffering, automatic production-data rotation/deletion/truncation, or premature DB/index migration.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff. Arduino owns SSR. ESP32/Web never controls SSR directly. `RUN_COMPLETED` is evidence only. Exact `spool_id` + `source_session_id` + `source_run_id` provenance remains mandatory for wire write-off. Restore remains operator-only and fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
