# Текущее состояние CoilMaster

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Рабочий исходный код только из `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

Production behavior подтверждён GREEN through checkpoint **165**. Checkpoint **166** закрыт как финальный residual audit **NO-CHANGE**: безопасных и достаточно значимых software-оптимизаций больше не найдено. **Software optimization complete. Следующий обязательный этап — full two-board Arduino + ESP32 hardware E2E acceptance.**

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
165 required target area derives from already-cached sourceArea; removes second source-component area pass
166 final residual audit -> NO-CHANGE; remaining candidates are cosmetic/low-value or require extra helper/flash complexity
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
e2d84e5ab37ec89724c8a1f71d5f29ddd62c5cea  checkpoint 164
db642c50a79d80179a765c5c4ff8ebb5006fd27f  checkpoint 165 final production code
```

## Latest verified CI evidence

```text
CMP Tests #3756    33047296869 / SUCCESS  (checkpoint 163 production)
ESP32 Build #1645  33047296953 / SUCCESS  (checkpoint 163 production)
CMP Tests #3759    33047621155 / SUCCESS  (checkpoint 164 production)
ESP32 Build #1647  33047621128 / SUCCESS  (checkpoint 164 production)
CMP Tests #3766    33047940015 / SUCCESS  (checkpoint 165 production)
ESP32 Build #1650  33047940040 / SUCCESS  (checkpoint 165 production)
CMP Tests #3767    33048020592 / SUCCESS  (checkpoint 165 handoff HEAD)
```

Intermediate ESP32 Build #1642 (`33046801931`) failed on commit `7936b8f9964294cf0164a8e5287cfbfdc19f8c7d`: `evaluateOption()` declaration had been expanded to 10 arguments while two call sites and the definition still used 8. The corrected checkpoint-162 production commit is `18d611e6ee8bb0355deda5f99874b0b9923d576f`, directly verified by CMP #3750 and ESP32 #1643.

CMP host audit remains 69 mandatory steps.

## Checkpoint 165 — GREEN

`ConductorCalculatorWeb::handleCalculate()` now computes `sourceSetAreaMicrometre2(source)` once. The already-cached `sourceArea` is passed to the area-based `requiredTargetAreaMicrometre2()` overload, removing the second source-component scan. Existing bundle/set overloads delegate to the same unchanged material-ratio formula. Integer rounding, overflow saturation, ranking and JSON semantics are unchanged. CMP #3766 and ESP32 #1650 directly verify production; CMP #3767 verifies the handoff HEAD.

## Checkpoint 166 — NO-CHANGE / software optimization complete

Final residual audit reviewed the remaining obvious bounded runtime candidates. `NetworkWeb::handleSave()` still has only isolated repeated `WebServer::arg()` retrievals for optional `id`/`password`; `WarehouseWeb::handleListSpools()` similarly repeats the optional diameter request value before delegating to the shared server-based parser. Removing these would save only a few request-string retrievals while requiring extra local/helper code and likely flash growth. No remaining candidate justifies another production change. Previously rejected safety-sensitive multi-pass scans remain intentionally unchanged.

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

1. Freeze software optimization at checkpoint **166 NO-CHANGE** unless hardware E2E exposes a concrete defect.
2. Perform full two-board Arduino + ESP32 hardware E2E acceptance.
3. Verify the production flow end to end, including physical START ownership, run lifecycle, recovery/fail-closed behavior, exact-spool provenance and manual RUN_WIRE write-off.
4. Record measured Uno/ESP32 build/runtime evidence and any hardware findings in PROJECT_HANDOFF.
5. After hardware E2E passes, prepare final release-completion checkpoint.

## Hardware acceptance

**Now required.** Full two-board Arduino + ESP32 E2E is the remaining acceptance gate before final project completion.
