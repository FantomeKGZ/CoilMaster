# Активная работа и следующие шаги

Дата обновления: **2026-08-28**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Текущая рабочая ветка: **`arduino-ru-lcd-experiment`**

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

Latest direct verification of the old production baseline:

```text
CMP Tests #3759    33047621155 / SUCCESS  (checkpoint 164 production)
ESP32 Build #1647  33047621128 / SUCCESS  (checkpoint 164 production)
CMP Tests #3766    33047940015 / SUCCESS  (checkpoint 165 production)
ESP32 Build #1650  33047940040 / SUCCESS  (checkpoint 165 production)
CMP Tests #3767    33048020592 / SUCCESS  (checkpoint 165 handoff HEAD)
```

## 2026-08-28 — experiment -> production transfer COMPLETED

The previous planned transfer was re-checked and then executed as a **non-force fast-forward**.

Verified transfer snapshot:

```text
cmp-protocol-v1              28c7917a906bc9b15736369e8986d0e0c354ab8c
arduino-ru-lcd-experiment    28c7917a906bc9b15736369e8986d0e0c354ab8c
compare                      identical / ahead=0 / behind=0
```

So all changes that existed in `arduino-ru-lcd-experiment` through `28c7917a906bc9b15736369e8986d0e0c354ab8c`, including the new repair-material plan, are now present in `cmp-protocol-v1`.

Immediately after the fast-forward, production CMP Protocol Tests #3772 / run `33141922657` completed **FAILURE** on the same SHA. Build/test core steps passed, but three contract audits failed:

1. `Audit motor schema UI contracts`;
2. `Audit motor details and repair history contracts`;
3. `Audit Hall calibration safety contracts`.

Therefore the transferred production state must **not** be called GREEN yet. These three concrete regressions are the first priority in the next work session. Do not revert the whole transfer blindly; inspect the failing contract scripts and the current branch files to distinguish intentional experiment/UI evolution from real safety/schema regressions.

## New active software block — repair materials and write-off

Authoritative implementation plan:

`docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md`

The new block covers:

1. unified repair-material card;
2. warehouse selector with stock/price visibility;
3. copper/aluminium wire via exact `spool_id`;
4. bearings and general consumables;
5. explicit/manual write-off only;
6. price snapshot and repair costing integration;
7. append-only correction/history model;
8. actionable fail-closed UX errors;
9. desktop/mobile parity;
10. duplicate-submit, stale-preview/TOCTOU, insufficient-stock, provenance and recovery tests.

## Current execution order

1. Continue work only in `arduino-ru-lcd-experiment` from the synchronized production snapshot.
2. Inspect and resolve the three CMP #3772 contract failures without weakening safety invariants.
3. Re-run/verify relevant CI; do not declare the synchronized baseline GREEN before confirmed success.
4. Then perform a read-only audit of MaterialLedger, Warehouse, Repair costing, RUN_WIRE APIs and desktop/mobile UI.
5. Produce the exact current data-flow map and minimum extension points.
6. Implement the repair-material plan in small checkpoints with tests and handoff updates.
7. Do not copy further experiment commits into `cmp-protocol-v1` until a new explicit transfer is agreed.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff. Arduino owns SSR. ESP32/Web never controls SSR directly. `RUN_COMPLETED` is evidence only. Exact `spool_id` + `source_session_id` + `source_run_id` provenance remains mandatory for wire write-off. Restore remains operator-only and fail-closed.

MaterialLedger `confirmUsage()` remains intentionally two-pass across the pre-WAL snapshot and mutation-time authoritative reread. Different integrity ledgers and mutation phases remain separate. No tail-only historical integrity replacement, unbounded buffering, automatic data truncation/rotation or premature DB/index migration is justified.

## Hardware acceptance

Full two-board Arduino + ESP32 E2E remains required before final project completion. For the new materials software block, hardware E2E may remain deferred until software checkpoints are complete unless a concrete hardware-dependent defect requires earlier verification.
