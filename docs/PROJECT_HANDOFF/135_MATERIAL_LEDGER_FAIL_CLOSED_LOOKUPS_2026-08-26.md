# Checkpoint 135 — MaterialLedger fail-closed lookup narrowing

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Scope

MaterialLedger read APIs were narrowed so public callers cannot collapse storage/integrity failure into a normal not-found result.

## Changes

- Public `repairExists(uint32_t repairId, bool& found)` remains authoritative.
- One-argument `repairExists(uint32_t repairId)` moved behind `private:` and is retained only for the current internal `confirmUsage()` path.
- Removed dead one-argument `loadActiveMaterialState(materialId, state)` wrapper.
- Removed dead one-argument `loadActiveMaterialCurrency(materialId, currency)` wrapper.
- Public material state/currency reads now require explicit `found` output.
- Existing full schema validation, ACTIVE-only semantics, KGS policy and wire metadata validation are unchanged.

Public result boundary:

```text
false              = storage/integrity/read failure
true + found=false = valid read, requested active record absent
true + found=true  = valid active record returned
```

## Evidence

```text
6d1ca9a32611b0d0fc42ce4ed2aa1aa22e5d98d9  narrow MaterialLedger header surface
4f92afd94aec4eca0c2fda4ec6ad7d13c9065e9c  remove dead state/currency wrappers
1c9e2c6b402b28130ffd9e67d12a29b6918476e2  fail-closed lookup contracts
ESP32 Build #1587   32971695182 / SUCCESS
CMP Tests #3601     32971743951 / SUCCESS
```

## Follow-up audit result

`MaterialLedgerWeb::handleUsage()` intentionally retains its read-only material preflight before `confirmUsage()`: the Web pass distinguishes `material_not_found` / unsupported currency for HTTP semantics, while `confirmUsage()` must re-read authoritative state immediately before mutation to remain fail-closed against TOCTOU. This is not a safe duplicate-pass removal candidate.

## Safety

No automatic START, resume or writeoff was introduced. `RUN_COMPLETED` remains evidence only. No storage rotation/deletion/truncation or database migration was added.
