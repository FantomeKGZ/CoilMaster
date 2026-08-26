# Checkpoint 136 — MaterialLedger dead adjustment helper removal

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Scope

Removed the unused private `adjustmentExists(uint32_t)` helper and its full `adjustments.ndjson` scan. Search showed no production or recovery caller outside declaration/definition.

## Changes

- Removed `adjustmentExists(uint32_t adjustmentId) const` from `CM_MaterialLedger.h`.
- Removed its implementation from `CM_MaterialAdjustment.cpp`.
- Adjustment transaction/recovery logic remains unchanged:
  - durable audit detection remains inline in `recoverPendingAdjustment()`;
  - BEFORE/AFTER material-state reconciliation remains fail-closed;
  - audit append and pending removal ordering remains unchanged.

## Evidence

```text
936221f0398dd60e30a77deb3552ca823eaa8bd2  declaration removed
9dcd1948f7ebe5b1d3d079d6806ae910d5f39109  dead full-log helper removed
277bddc277155a6d097baae00270fcd8fd445d13  atomic-recovery contract updated
ESP32 Build #1589   32972261210 / SUCCESS
CMP Tests #3608     32972305473 / SUCCESS
```

## NEXT

The remaining dead/private helpers in the large `CM_MaterialLedger.cpp` (`usageExists`, `restoreQuantity`, plus the private one-argument repair wrapper) should be removed only together in one exact full-file rewrite, with `confirmUsage()` switched to explicit `repairExists(..., found)`.

No automatic START/resume/writeoff or production-data cleanup/rotation was introduced.
