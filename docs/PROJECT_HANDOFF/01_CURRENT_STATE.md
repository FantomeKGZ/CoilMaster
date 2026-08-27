# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Рабочий исходный код только из `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

Production behavior подтверждён GREEN through checkpoint **163**. Checkpoint **164** реализован и находится под CI. Hardware E2E пока не требуется; продолжается repo-reviewable software optimization.

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
157 material history optional query values fetched once
158 material adjustment optional quantity/price values fetched once
159 standard conductor recommendations reuse one warehouse availability lookup per component
160 calculator warehouse diameter lookup -> binary search over sorted fixed catalogue
161 warehouse catalogue stays sorted during scan -> binary lookup + bounded insertion, no final O(N^2) sort
162 recommendation search reuses precomputed single-wire areas across strand combinations
163 recommendation top-3 caches rankingScore; existing scores are not recalculated per candidate
164 calculator Web request reuses one required target area for warehouse + standard searches + JSON
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
ac5411cc7ad2f279eef655fc3b0e3be3f139b4d0  checkpoint 163
e2d84e5ab37ec89724c8a1f71d5f29ddd62c5cea  checkpoint 164 current production
```

## Latest verified CI evidence

```text
CMP Tests #3750    33046851201 / SUCCESS  (checkpoint 162 production)
ESP32 Build #1643  33046851156 / SUCCESS  (checkpoint 162 production)
CMP Tests #3756    33047296869 / SUCCESS  (checkpoint 163 production)
ESP32 Build #1645  33047296953 / SUCCESS  (checkpoint 163 production)
CMP Tests #3757    33047347514 / SUCCESS  (checkpoint 163 handoff HEAD 93e4432...)
```

Checkpoint 164 CI started on `e2d84e5a...`:

```text
CMP Tests #3759    33047621155 / in_progress at first direct check
ESP32 Build #1647  33047621128 / in_progress at first direct check
```

Intermediate ESP32 Build #1642 (`33046801931`) failed on commit `7936b8f9964294cf0164a8e5287cfbfdc19f8c7d`: `evaluateOption()` declaration had been expanded to 10 arguments while two call sites and the definition still used 8. The corrected checkpoint-162 production commit is `18d611e6ee8bb0355deda5f99874b0b9923d576f`, directly verified by CMP #3750 and ESP32 #1643.

CMP host audit remains 69 mandatory steps.

## Checkpoint 163 — GREEN

`ConversionOption` carries an internal `rankingScore`, computed once for each accepted candidate. Bounded top-3 insertion compares cached scores instead of recomputing `optionScore()` for already-selected entries on every candidate. Ranking formula, strict `<` tie behavior and output ordering remain unchanged. Fixed RAM cost is at most 12 bytes across the three returned options; no heap or unbounded storage is introduced. CMP #3756 and ESP32 #1645 directly verify production; CMP #3757 verifies the handoff HEAD.

## Checkpoint 164

`ConductorCalculatorWeb::handleCalculate()` previously invoked source-based recommendation search separately for warehouse and standard catalogues, then recalculated source/required areas again for response JSON. `findRecommendedOptionsForArea()` is now public, and the Web handler computes `requiredArea` once per request and reuses that exact value for both recommendation searches and JSON output. `sourceArea` is also cached for response serialization. Conversion formulas, ranking, catalogue selection, HTTP errors and JSON values remain unchanged; no unbounded memory is introduced.

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

1. Confirm CMP #3759 and ESP32 Build #1647 on `e2d84e5a...`; checkpoint 164 is not GREEN until both are directly successful.
2. Then start checkpoint **165** from current `cmp-protocol-v1` HEAD.
3. Continue only with measurable runtime/storage/flash wins and fixed-memory behavior.
4. Preserve complete authoritative validation, HTTP preflight behavior, mutation-time TOCTOU checks, exact-spool provenance and deterministic recovery.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
