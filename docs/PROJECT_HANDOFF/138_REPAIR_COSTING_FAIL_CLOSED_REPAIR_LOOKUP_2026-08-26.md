# Checkpoint 138 — RepairCosting fail-closed repair lookup

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Scope

Removed the ambiguous one-argument repair lookup from `RepairCosting` and kept the existing single-pass pricing-save ownership intact.

## Changes

- Removed `repairExists(uint32_t repairId)` declaration and implementation.
- Retained only `repairExists(uint32_t repairId, bool& found)`.
- `RepairCosting::load()` now explicitly initializes `repairFound` and rejects either read/integrity failure or `found=false`.
- `savePricing()` still delegates repair identity validation to `load()` and does not add a duplicate `repairs.ndjson` scan.
- Movement costing, RUN_WIRE de-duplication, pricing append and currency semantics are unchanged.

## Evidence

```text
6267571a6cc2704ce2769a3d1f5f6f8c909575a2  header wrapper removed
e4771124b8f65f3dfb144a9569d6cc9ba83488b4  validation wrapper removed
3c62d73d3cbd24fe08013cee63a59a8353af3e50  load uses explicit found
959f8d283e1669f083d9361b11af9a194154fee4  single-pass/fail-closed contract
ESP32 Build #1595   32973529504 / SUCCESS
CMP Tests #3622     32973582094 / SUCCESS
```

The source-only CMP run failed because the textual contract still expected the retired wrapper; final integrated CMP #3622 is GREEN.

## Safety

No automatic START/resume/writeoff was introduced. RUN_WIRE accounting ownership and exact provenance remain unchanged. No production-data deletion/rotation/truncation was added.
