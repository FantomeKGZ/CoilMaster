# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 165; checkpoint 166 NO-CHANGE; software optimization complete

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
163 recommendation top-3 caches rankingScore; existing scores are no longer recalculated per candidate
164 calculator Web request reuses one required target area for warehouse + standard searches + JSON
165 required target area derives from already-cached sourceArea; removes second source-component area pass
166 final residual audit -> NO-CHANGE; no remaining safe meaningful software optimization
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

Latest direct verification:

```text
CMP Tests #3759    33047621155 / SUCCESS  (checkpoint 164 production)
ESP32 Build #1647  33047621128 / SUCCESS  (checkpoint 164 production)
CMP Tests #3766    33047940015 / SUCCESS  (checkpoint 165 production)
ESP32 Build #1650  33047940040 / SUCCESS  (checkpoint 165 production)
CMP Tests #3767    33048020592 / SUCCESS  (checkpoint 165 handoff HEAD)
```

Intermediate ESP32 Build #1642 (`33046801931`) failed on `7936b8f9964294cf0164a8e5287cfbfdc19f8c7d` because the expanded `evaluateOption()` declaration expected 10 arguments while two call sites and the definition still used 8. The corrected checkpoint-162 production commit is `18d611e6ee8bb0355deda5f99874b0b9923d576f`, verified by CMP #3750 + ESP32 #1643.

CMP host audit remains 69 mandatory steps.

## Checkpoint 165 — GREEN

The Web handler computes `sourceSetAreaMicrometre2(source)` once and passes the cached `sourceArea` to an area-based `requiredTargetAreaMicrometre2()` overload. Existing bundle/set overloads delegate to the same material-ratio formula. The second source-component area scan is removed while integer rounding, overflow saturation, material conversion, recommendation ranking and JSON values remain unchanged. CMP #3766 and ESP32 #1650 directly verify production; CMP #3767 verifies the handoff HEAD.

## Checkpoint 166 — NO-CHANGE

Final residual audit reviewed remaining bounded runtime candidates. `NetworkWeb::handleSave()` has only isolated repeated `WebServer::arg()` retrievals for optional `id`/`password`. `WarehouseWeb::handleListSpools()` similarly checks the optional diameter String once before delegating to the existing server-based numeric parser. Eliminating these remaining micro-retrievals would require additional local/helper code and likely flash growth for negligible runtime benefit. The previously rejected repair-status request-wide scan remains unsafe without bounded indexing because repair close order is not guaranteed to follow `repair_id`.

MaterialLedger `confirmUsage()` remains intentionally two-pass across the pre-WAL snapshot and mutation-time authoritative reread. Different integrity ledgers and mutation phases remain separate. No tail-only historical integrity replacement, unbounded buffering, automatic data truncation/rotation or premature DB/index migration is justified.

**Conclusion: software optimization complete. Do not force further software changes unless hardware E2E exposes a concrete defect.**

## Current active queue — hardware E2E acceptance

1. Flash/use the current `cmp-protocol-v1` production firmware on Arduino Uno and ESP32.
2. Perform full two-board UART/production-flow E2E from ESP32 command/assignment through Arduino acknowledgement and required physical START.
3. Verify no automatic START or auto-resume after reboot; Arduino remains sole SSR owner.
4. Verify RUN_STARTED/RUN_COMPLETED lifecycle and recovery/fail-closed behavior.
5. Verify exact `spool_id` + `source_session_id` + `source_run_id` provenance and explicit/manual RUN_WIRE write-off only.
6. Verify key operator paths affected by prior hardware pressure: LCD, keypad, START button, UART and Hall/calibration architecture behavior.
7. Record measured Uno flash/RAM, ESP32 build/runtime evidence and all hardware findings in PROJECT_HANDOFF.
8. If hardware E2E passes, prepare final release-completion checkpoint. If it exposes a concrete defect, fix only that defect and re-run the relevant acceptance path.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff. Arduino owns SSR. ESP32/Web never controls SSR directly. `RUN_COMPLETED` is evidence only. Exact `spool_id` + `source_session_id` + `source_run_id` provenance remains mandatory for wire write-off. Restore remains operator-only and fail-closed.

## Hardware acceptance

**Now required.** Full two-board Arduino + ESP32 E2E is the remaining acceptance gate before final project completion.
