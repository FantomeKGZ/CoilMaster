# Checkpoint 137 — MaterialLedger retired private helpers removal

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Scope

Completed the staged MaterialLedger API cleanup started in checkpoints 135–136.

## Changes

- `confirmUsage()` now uses only `repairExists(repairId, found)` and explicitly rejects both read failure and `found=false`.
- Removed the last one-argument `repairExists(repairId)` wrapper and implementation.
- Removed dead private `usageExists()` full-log helper.
- Removed dead private `restoreQuantity()` catalog rewrite helper.
- Kept `readStockQuantity()` because pending-usage recovery still requires exact BEFORE/AFTER stock reconciliation.
- Existing pending transaction ordering, material mutation rewrite, KGS gate and deterministic recovery remain unchanged.

## Evidence

```text
32a9ce4ee403ac9cce56b36a570b19f45ba510fd  retired helper declarations removed
78b7719a962912417e86c96ac14bd441f4d67d1d  one-arg repair implementation removed
90c732a9caef1d1e4104c9c7374a72f6a8df3811  confirmUsage explicit found + dead core helpers removed
aecb1a9ff8721c7eafd3741762e7ff7d4854e413  explicit lookup contract
5dc6f1c0303834274d989b8846b53ba34c1f3368  usage/recovery/dead-helper contract
ESP32 Build #1592   32972822029 / SUCCESS
CMP Tests #3614     32972911974 / SUCCESS
```

The earlier CMP run on the source-only commit failed because text contracts had not yet been updated; final CMP #3614 is the authoritative integrated result.

## Safety

No automatic physical START, repeat START, resume or wire writeoff was introduced. `RUN_COMPLETED` remains evidence only. No production-data deletion, truncation, rotation or DB migration was added.
