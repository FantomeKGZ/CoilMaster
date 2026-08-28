# Текущее состояние CoilMaster

Дата обновления: **2026-08-28**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Текущая рабочая ветка: **`arduino-ru-lcd-experiment`**

## Source of truth / branch policy

Production остаётся на `cmp-protocol-v1`; `main` для исходников не использовать.

Текущую дальнейшую разработку и experiment-side optimization выполнять только в `arduino-ru-lcd-experiment`. Не переносить experiment обратно в production без отдельного прямого запроса пользователя.

Production не изменён:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

## Current phase

Production behavior подтверждён GREEN through checkpoint **165**. Production checkpoint **166** закрыт как residual audit **NO-CHANGE** и production software optimization остаётся frozen до hardware E2E либо конкретного дефекта.

После синхронизации production дальнейшие изменения продолжаются отдельно в `arduino-ru-lcd-experiment`. Experiment-side repeated-scan/performance work подтверждён through checkpoint **157**. Full two-board Arduino + ESP32 hardware E2E остаётся отдельным финальным acceptance gate.

## Production optimization checkpoints

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
166 final residual audit -> NO-CHANGE
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

## Latest verified production CI evidence

```text
CMP Tests #3766    33047940015 / SUCCESS
ESP32 Build #1650  33047940040 / SUCCESS
CMP Tests #3767    33048020592 / SUCCESS
```

## Experiment checkpoints 152–157

Experiment branch currently includes the separate RUN_WIRE Material Request batching, unified autonomous/Web archive, exact immutable-spool lookup and subsequent repeated-scan reductions documented in `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

Latest experiment checkpoint **157 — GREEN**:

- `CashPaymentWeb::loadClientCharges()` no longer calls generic `RepairCosting::load()` for every repair and therefore no longer performs one full `repairs.ndjson` validation per repair;
- one authoritative full `RepairCosting::repairExists()` validation is performed once after the first repair id is obtained from the bounded RepairRegistry client page;
- subsequent ids in the same read-only client-balance request use `RepairCosting::loadKnownRepair()`;
- generic `RepairCosting::load()` still performs exact `repairExists()` validation before delegating;
- mutation path `savePricing()` still uses generic `load()` and is unchanged;
- all costing journals, RUN_WIRE pending guard, Warehouse movement integrity, material usage/correction integrity, currency/overflow validation remain unchanged;
- no cache/index/DB, whole-file buffering or unbounded collection added.

Runtime commits:

```text
e50b09bc61ca3f3a4a053a5dd826f25d36ad6c71  add known-repair costing read path
347ca06061129afe223e6fa56b7379315cfca38b  generic load validates then delegates
bf529900a8211f0b9a920ec237942bae2f7093c5  client balance validates repairs once
```

Runtime CI for `bf529900a8211f0b9a920ec237942bae2f7093c5`:

```text
CMP Protocol Tests #3939  run 33191506843 / SUCCESS
ESP32 Build #1743         run 33191506805 / SUCCESS
Arduino RU LCD #167       run 33191506861 / SUCCESS
```

Final regression contract:

```text
19bf03003b5e1f3aea6692609e634a79247fb397
CMP Protocol Tests #3941  run 33194910481 / SUCCESS
```

Intermediate contract commit `6a5ecda6eb514f65152d4d6ff1b00a7f995f109d` produced CMP #3940 FAILURE only because two text assertions expected different local variable/index spelling; host build/CTest and all other executed audits were healthy. The assertion-only correction is `19bf0300...`; runtime code was not changed.

The adjacent CashPayment correction preflight was audited in the same block and remains **NO-CHANGE**: combining `eventBelongsToRepair()` and `totalsForRepair()` would require a parallel validation implementation and risk changing current fail-closed/error semantics. Mutation-time `append()` authoritative scan remains mandatory.

## Safety / integrity boundaries that remain intentionally unchanged

- No automatic physical START, repeat START or resume.
- Arduino owns SSR; ESP32/Web never drives SSR directly.
- `RUN_COMPLETED` is evidence only and never automatically deducts wire.
- Wire write-off remains explicit/manual and tied to exact `spool_id`, `source_session_id`, `source_run_id`.
- MaterialLedger `confirmUsage()` keeps separate pre-WAL and mutation-time authoritative reads.
- Different ledgers / different mutation phases keep separate validation passes when they are integrity boundaries.
- No tail-only substitute for authoritative historical integrity scans.
- No unbounded vectors or whole-file buffering for growing NDJSON.
- No automatic production-data rotation/deletion/truncation and no premature DB/index migration.

## Current NEXT

1. Continue only in `arduino-ru-lcd-experiment`; production remains unchanged.
2. Continue repo-reviewable repeated-scan/performance audit only where the same request repeats an authoritative growing-journal pass and the proof can be preserved.
3. Next inspect adjacent client-balance/costing aggregation for remaining N-per-repair journal work; do not introduce batch state unless validation semantics remain equivalent and RAM stays bounded.
4. Keep CashPayment correction preflight as NO-CHANGE unless a simpler proof-preserving one-pass primitive appears.
5. Full two-board Arduino + ESP32 hardware E2E remains the final separate acceptance gate before final release completion.

## Hardware acceptance

Full two-board Arduino + ESP32 E2E remains required before final project completion, but current repo-reviewable experiment optimization may continue without intermediate hardware tests.
