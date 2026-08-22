# Job-state dismiss cleanup — 2026-08-22

Ветка: `cmp-protocol-v1`

## Classification

`firmware/esp32/src/CM_JobStateDismiss.cpp` — **DELETE**.

Удалённый метод:

```cpp
JobStateStore::dismissInactive(uint32_t sessionId, uint32_t nowMs)
```

## Dependency proof

Перед удалением были проверены текущие owner-ы job-state/recovery:

- `CM_JobRecovery.cpp` — reboot recovery остаётся fail-closed; `ClosedAfterReview` не восстанавливается как активное задание и auto-queue/auto-resume запрещены;
- `CM_JobStateRemoteCancel.cpp` — положительно подтверждённый Arduino cancel продолжает закрываться отдельным `closeAfterRemoteCancel()`;
- `CM_JobStateLateRunRecovery.cpp` — lost-JOB_ACK reconciliation остаётся отдельным узким `confirmStartedAfterDeliveryTimeout()`;
- `CM_JobStateStore.h` — manual-review closure остаётся через `closeAfterManualReview()`;
- current repository search по точному `dismissInactive(` не нашёл caller-ов. Пустой search не использовался как единственное доказательство: решение основано на прямом owner/API inspection выше.

`dismissInactive()` не владел hardware action, UART delivery, SSR, START, run evidence, material writeoff или recovery safety. Он был отдельным неиспользуемым convenience API, переводившим уже terminal/completed runtime state в `ClosedAfterReview`.

## Changes

```text
778bc9f23e70631580c54273e16d11e218295f25
  Remove unused job-state dismiss API
  - removed public declaration from CM_JobStateStore.h

39142bd9834e26e349faf0ff4b2a6cb3076777f0
  Remove unused job-state dismiss implementation
  - removed CM_JobStateDismiss.cpp
```

## Adjacent owner sweep

The nearby validation split was also checked instead of being removed by similarity alone:

- `CM_WarehouseRepairValidation.cpp` — **KEEP**. `WarehouseStore::repairExists(...)` is used by the manual warehouse writeoff path to validate the exact repair reference before mutation.
- `CM_RepairCostingValidation.cpp` — **KEEP**. `RepairCostingWeb` directly calls `m_costing.repairExists(...)` for `/api/repairs/costing` and `/api/repairs/pricing-history` before reading or exposing costing/history data.
- These implementations intentionally belong to different storage/service owners. Their similar validation loops are not sufficient evidence for deletion or premature cross-owner merge.
- `CM_WarehousePrice.cpp` — **KEEP**. It owns fail-closed warehouse price lookup used by current warehouse/material costing behavior.

## Safety invariants preserved

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web still do not directly control SSR;
- lost ACK/timeout still does not prove Arduino idle;
- manual review closure remains explicit;
- remote cancel closure still requires positive Arduino cancellation acknowledgement;
- immutable snapshot/run/history evidence is not deleted;
- `RUN_COMPLETED` still does not automatically deduct wire/material.

## Verification state

At the time of this checkpoint, no fresh successful CI/status check for commit `39142bd9834e26e349faf0ff4b2a6cb3076777f0` was available through the connector. Therefore this cleanup is **not recorded as GREEN yet**.

Next verification gate: fresh applicable ESP32 build + protocol/static contracts, or explicit operator-confirmed GREEN evidence.
