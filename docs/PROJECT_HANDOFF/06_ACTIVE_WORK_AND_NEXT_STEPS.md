# Активная работа и следующие шаги

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

Этот файл содержит только текущую активную очередь. Старые checkpoints — history/evidence, а не backlog.

## Current verified software baseline

Latest fully verified implementation block: **92_MATERIAL_USAGE_SINGLE_PASS_PREFLIGHT**.

```text
72401aae0d1b34fbb211ce92c48d0a367f337b91  perf implementation
8ce55052f98d491f3f1f2fda4830955e87159798  regression guard
6d77ac1b4ad7fcc25cc1873d5e0c13e819011ece  CMP workflow wiring

ESP32 Build #1443 / run 32831517073 / SUCCESS
CMP #3111 / run 32831517018 / SUCCESS
CMP #3112 / run 32831547926 / SUCCESS
CMP #3113 / run 32831593193 / SUCCESS
CMP #3114 / run 32831715701 / SUCCESS
CMP #3115 / run 32831755533 / SUCCESS
CMP #3116 / run 32831820366 / SUCCESS
CMP #3117 / run 32831861941 / SUCCESS
```

Block 92 collapses two equivalent preflight reads of `/data/materials/materials.ndjson` in `MaterialLedger::confirmUsage()` into one authoritative `readMaterialState()` pass while retaining `rewriteQuantity()` as the separate transactional mutation/revalidation pass.

Overall project readiness: **~95%**. Software/repo implementation and integrity: **~98-99%**.

## Active phase

**Stage-1 ESP32/storage software optimization is closed.**

Closure checkpoint:

```text
docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md
```

The final review found no additional justified duplicate scan/O(n*m) rewrite that can be removed without weakening HTTP diagnostics, transaction boundaries, fail-closed integrity, bounded RAM, or exact provenance.

Do not continue optimization merely to reduce scan count. Reopen repo-only performance work only with new measured evidence or a newly demonstrated true semantic duplicate.

## Current KEEP

Do not reopen without new evidence:

- `MaterialLedgerWeb::handleUsage()` HTTP prevalidation vs core mutation validation;
- warehouse write-off Web prevalidation vs core/transaction revalidation;
- warehouse price Web no-op detection vs service-level integrity/no-op protection;
- repair-status bounded self-scan;
- autonomous assignment event batching;
- warehouse movement provenance uniqueness batching;
- legacy ESP32 `CAL_RESULT` receive fallback;
- Uno resource-sensitive buffers/contracts already classified KEEP.

## NDJSON rule

- no premature DB migration;
- no automatic cleanup/rotation;
- no production-data truncation;
- `/api/system/storage` observability is available;
- threshold/rotation decisions only after real-device measurements.

## Safety invariants

Never weaken:

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never controls SSR directly;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- RUN_COMPLETED never auto-writes off wire/material;
- linked-production writeoff remains manual and exact `source_session_id + source_run_id + immutable spool_id`;
- cancellation never erases immutable run/history evidence;
- restore is operator-only, transactional, fail-closed.

## Next mandatory step — final two-board hardware acceptance

Perform one complete ESP32 + Arduino Uno acceptance; this is the final external hardware gate, not an intermediate test.

```text
1. boot both boards / CMP1 handshake
2. linked JOB delivery; Uno receives it but does not auto-start
3. physical START only
4. RUN_STARTED reaches ESP32
5. stable normal Hall count
6. RUN_COMPLETED reaches ESP32
7. no automatic wire/material writeoff
8. manual exact source_session_id + source_run_id + immutable spool_id writeoff
9. repeat requires another physical START; final repeat cannot auto-reopen
10. cancel/recovery
11. reboot -> no auto-resume / no physical auto-start
12. Hall calibration full flow: CAL_ARM -> local confirm -> physical START -> CAL_SAMPLE/CAL_DONE -> proposal -> local apply -> CFG reconciliation
13. keypad/LCD/buzzer usable
14. SSR remains Uno-only
```

After this hardware acceptance, use real device storage metrics to decide whether any NDJSON threshold/rotation work is actually needed.

## New-chat entrypoint

Read first:

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md
docs/PROJECT_HANDOFF/92_MATERIAL_USAGE_SINGLE_PASS_PREFLIGHT_2026-08-25.md
```

`90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md` remains the authoritative transfer checkpoint. `93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md` records the software-stage closure and the transition to final hardware acceptance.
