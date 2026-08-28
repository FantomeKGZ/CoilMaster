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

So all changes that existed in `arduino-ru-lcd-experiment` through `28c7917a906bc9b15736369e8986d0e0c354ab8c`, including the new repair-material plan, are present in `cmp-protocol-v1`.

Production CMP Protocol Tests #3772 / run `33141922657` completed **FAILURE** on production SHA `28c7917a906bc9b15736369e8986d0e0c354ab8c`. Configure/build/ctest passed. Four actual failing audit steps were identified from the job result/logs:

1. `Audit motor schema UI contracts`;
2. `Audit motor details and repair history contracts`;
3. `Audit Hall calibration safety contracts`;
4. `Audit Hall raw migration ownership`.

Read-only comparison with current experiment behavior proved these were stale contract assertions, not a reason to roll back the transferred runtime:

- motor CRUD had intentionally moved to dedicated create/edit pages, while old assertions still expected inline catalog/repair quick-add fields;
- motor role UI had intentionally localized WORKING/STARTING labels while exact `role=working` / `role=starting` navigation remained intact;
- Hall experiment intentionally starts calibration motor motion only from local keypad `A` or the physical START input after baseline readiness, while EEPROM apply still requires separate local `#` confirmation and Web has no motor-start/SSR endpoint;
- Uno compact completion intentionally delegates CRC append to shared `CrcFrameText::append`, while the old ownership audit required the previous direct `Cmp1Crc::calculate` literal.

Only contract tests were aligned; Hall/SSR runtime safety was not weakened. Fix commits in `arduino-ru-lcd-experiment`:

```text
bb4aa5c58b173f015df0b8a3971a28a4ece62d9c  motor schema audit -> dedicated CRUD pages
c73cf2164ccbc3c9c7c232d6778789037704ddb9  motor details audit -> localized role UI
6ac4de83ba95659a679a0dc171517f9f08aa61d1  Hall safety audit -> local A/physical START experiment contract
9b239f30e991876b77ec2f8f972d9c82b7616872  Hall raw migration audit -> shared CRC formatter
```

A CI coverage gap was also closed: `CMP Protocol Tests` push trigger now includes `arduino-ru-lcd-experiment` in addition to production, while `main` remains excluded. Commit:

```text
ea56a53e67500cbee019321542446107e0bc316d  ci: run CMP contract suite on experiment branch
```

Direct verification of this experiment checkpoint:

```text
CMP Protocol Tests #3773
run 33147497236
SHA ea56a53e67500cbee019321542446107e0bc316d
status completed
conclusion success
```

All four previously failing audit steps completed `success` in #3773, as did Configure / Build / Test and the rest of the CMP host audit suite. This establishes a GREEN **experiment checkpoint** for the repaired contracts. It does **not** change production: `cmp-protocol-v1` remains at `28c7917a906bc9b15736369e8986d0e0c354ab8c` until a separate explicit transfer request.

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

1. Continue work only in `arduino-ru-lcd-experiment`.
2. Perform the planned read-only audit of MaterialLedger, Warehouse/spool storage, Repair costing, RUN_WIRE, material request/catalog, related APIs, desktop/mobile UI and backup/integrity coverage.
3. Produce the exact current data-flow map `warehouse/material/spool -> selection -> repair/run -> confirmation -> ledger -> costing -> history/backup` and identify reusable authoritative components.
4. Implement the minimum safe first repair-material checkpoint without creating a parallel ledger.
5. Verify relevant CI and update PROJECT_HANDOFF after each completed checkpoint.
6. Do not copy further experiment commits into `cmp-protocol-v1` until a new explicit transfer is agreed.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff. Arduino owns SSR. ESP32/Web never controls SSR directly. `RUN_COMPLETED` is evidence only. Exact `spool_id` + `source_session_id` + `source_run_id` provenance remains mandatory for wire write-off. Restore remains operator-only and fail-closed.

MaterialLedger `confirmUsage()` remains intentionally two-pass across the pre-WAL snapshot and mutation-time authoritative reread. Different integrity ledgers and mutation phases remain separate. No tail-only historical integrity replacement, unbounded buffering, automatic data truncation/rotation or premature DB/index migration is justified.

## Hardware acceptance

Full two-board Arduino + ESP32 E2E remains required before final project completion. For the new materials software block, hardware E2E may remain deferred until software checkpoints are complete unless a concrete hardware-dependent defect requires earlier verification.
