# CoilMaster — Stage-1 software optimization closure

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

## Status

Stage-1 ESP32/storage repo-only optimization phase is **CLOSED**.

Последний justified implementation block: **92_MATERIAL_USAGE_SINGLE_PASS_PREFLIGHT**.

```text
72401aae0d1b34fbb211ce92c48d0a367f337b91  perf(esp32): collapse material usage preflight scan
8ce55052f98d491f3f1f2fda4830955e87159798  regression guard
6d77ac1b4ad7fcc25cc1873d5e0c13e819011ece  CMP workflow wiring
```

Block 92 replaced two equivalent preflight material-catalog reads in `MaterialLedger::confirmUsage()` with one authoritative `readMaterialState()` pass while preserving the separate `rewriteQuantity()` mutation/revalidation pass.

## Verified CI evidence

Operator-reported and GitHub-verified successful tail on `cmp-protocol-v1`:

```text
CMP #3111 / run 32831517018 / SUCCESS
ESP32 Build #1443 / run 32831517073 / SUCCESS
CMP #3112 / run 32831547926 / SUCCESS
CMP #3113 / run 32831593193 / SUCCESS
CMP #3114 / run 32831715701 / SUCCESS
CMP #3115 / run 32831755533 / SUCCESS
CMP #3116 / run 32831820366 / SUCCESS
CMP #3117 / run 32831861941 / SUCCESS
```

`#3117` head is docs-only commit `b5666d86b31a2bc4c0274c4493023ad015ce221c` (`docs(handoff): point entrypoint to block 92`).

The firmware/software GREEN baseline remains the Block-92 implementation/test/CI chain above; later documentation commits do not represent a newer firmware implementation baseline.

## Final reviewed KEEP candidates

The remaining repeated reads were reviewed and intentionally retained because they enforce distinct semantics rather than duplicate work:

- `MaterialLedgerWeb::handleUsage()` pre-validates repair/material/lifecycle/currency for precise HTTP `404/409/500/503`; core `confirmUsage()` independently revalidates mutation safety.
- Warehouse write-off Web validation vs core mutation validation: exact repair/session/run/spool checks are repeated across API and transaction boundaries intentionally; spool state is rechecked during mutation to prevent stale/TOCTOU writes.
- Warehouse price Web no-op detection vs `WarehouseStore::setWarehousePrice()` service-level integrity/no-op protection are independent contracts.
- repair-status bounded self-scan remains KEEP.
- autonomous assignment event batching remains KEEP.
- warehouse movement provenance uniqueness batching remains KEEP.
- legacy ESP32 `CAL_RESULT` receive fallback remains KEEP.
- Uno resource-sensitive buffers/contracts remain KEEP unless new measured evidence appears.

## Stage-1 stop rule reached

No further repo-only rewrite is justified without new evidence such as:

1. measured device latency/storage-growth data;
2. a newly discovered true duplicate authoritative scan with equivalent semantics;
3. a proven O(n*m) reference lookup that can be bounded safely;
4. a concrete allocation/I/O regression.

Do not continue optimization merely to reduce scan count when the passes protect different API, integrity, recovery, or transaction semantics.

## Next mandatory gate

The next project step is **one full two-board hardware acceptance on ESP32 + Arduino Uno**. This is not an intermediate test; it is the final external hardware gate after software optimization closure.

Minimum acceptance sequence:

```text
1. Boot both boards and establish CMP1 communication.
2. Deliver one linked JOB from ESP32; Uno must receive it but must not auto-start.
3. Start only with the physical START control.
4. Verify RUN_STARTED reaches ESP32.
5. Verify normal Hall turn counting remains stable.
6. Verify RUN_COMPLETED reaches ESP32.
7. Verify RUN_COMPLETED does not automatically deduct wire/material.
8. Perform manual exact-run exact-spool writeoff using exact source_session_id + source_run_id + immutable spool_id.
9. Verify repeat requires another physical START and cannot auto-reopen after final repeat.
10. Verify cancel/recovery path.
11. Reboot and verify no auto-resume / no automatic physical start.
12. Run Hall calibration end-to-end: CAL_ARM -> local confirm -> separate physical START -> CAL_SAMPLE/CAL_DONE -> ESP32 proposal -> exact measurement_id -> local apply -> CFG reconciliation.
13. Verify keypad, LCD and buzzer remain usable.
14. Verify SSR authority remains exclusively on Arduino Uno.
```

## Safety invariants remain unchanged

- no automatic physical START;
- no automatic START between repeat cycles;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically writes off wire/material;
- writeoff remains manual and exact `source_session_id + source_run_id + immutable spool_id`;
- restore is operator-only, transactional, fail-closed;
- no automatic production-data deletion or NDJSON truncation.
