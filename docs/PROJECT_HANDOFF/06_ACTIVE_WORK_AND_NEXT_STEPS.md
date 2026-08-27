# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 158; checkpoint 159 under CI

```text
148 managed RUN_WIRE removes redundant spool pre-scan
149 spool/material bridge append -> one validated bridge-log pass
150 MaterialLedger confirmUsage two-pass retained as safety boundary
151 remaining append audit -> no safe same-ledger duplicate full scan
152 autonomous save -> one bounded-tail latest-event read
153 dead-helper linker audit -> NO-CHANGE; linker GC already strips them
154 autonomous task query parsed once per page
155 motor similarity candidate parsed once; each stored winding program parsed once
156 motor similarity Web handler reuses one coil_program request String
157 Material History optional query values fetched once and parsed from local String values
158 Material adjustment optional quantity/price values fetched once and parsed from local String values
159 standard conductor recommendations reuse one warehouse availability lookup per component
```

Production commits:

```text
1e831cde072f6c6152d10b7e71cb6a1e0f2a7b0e  checkpoint 152
a2f98cb377873d88d3fd103b6dfdfbabaf28ea65  checkpoint 154
415394d162de0f1c83e433cbbea3db94833b3162  checkpoint 155
78b41d38abdf89b9e72a02eea37edcd346c9610f  checkpoint 156
491fcf965fe573f91eb29bc99f6513017f3f5b1a  checkpoint 157
0596ae9ff473503bd1d21aeda0c6c4da0f2ba0da  checkpoint 158
93d858c83b3f63932d6c2809df585a017a74a6b6  checkpoint 159 current production HEAD
```

Latest direct verification:

```text
CMP Tests #3733    33043881805 / SUCCESS  (checkpoint 157 production)
ESP32 Build #1634  33043881890 / SUCCESS  (checkpoint 157 production)
CMP Tests #3734    33043911946 / SUCCESS  (checkpoint 157 handoff)
CMP Tests #3735    33044295465 / SUCCESS  (checkpoint 158 production)
ESP32 Build #1635  33044295425 / SUCCESS  (checkpoint 158 production)
CMP Tests #3736    33044341585 / SUCCESS  (checkpoint 158 handoff HEAD e4aa9c1...)
```

Checkpoint 159 verification started on `93d858c8...`:

```text
CMP Tests #3739    33044747417 / in_progress at last direct check
ESP32 Build #1638  33044747467 / in_progress at last direct check
```

CMP host audit remains 69 mandatory steps.

## Checkpoint 158

`MaterialLedgerWeb::handleAdjust()` fetches `add_quantity_milli` and `new_price_per_unit_minor` once and parses those already-held values through the shared `parseUnsignedValue()` path. Optional empty values keep their previous no-change semantics; malformed non-empty values remain `400`. `adjustMaterial()`, WAL ordering, mutation-time revalidation, recovery and RUN_WIRE are unchanged. CMP #3735 and ESP32 #1635 directly verify the production commit, so checkpoint 158 is GREEN.

## Checkpoint 159

The standard conductor recommendation response previously performed two linear warehouse-diameter searches per recommended component: once through `optionAvailableFromWarehouse()` to derive `warehouse_available`, and again while emitting each component's `available_g`. The current path performs one `availableGramsFor()` lookup per component into fixed `uint32_t[MaxConversionComponents]` storage, derives `warehouse_available` from those same values, and reuses them for JSON output. Maximum storage remains two fixed entries. Recommendation ranking, target-area/deviation calculations, warehouse catalogue loading, standard catalogue contents, response field order and HTTP behavior remain unchanged.

A proposed warehouse spool query parser refactor was rejected before checkpoint 159 because it would have added helper code for one filter and risked increasing flash. The temporary declaration commit was immediately reverted; the branch source after `174cc477...` is identical to the prior warehouse source.

The larger repair-page/status candidate remains intentionally unchanged. Repairs may close out of `repair_id` order, so lockstep `repairs.ndjson` + `repair-status.ndjson` streaming is invalid. A request-wide status scan would require unbounded retained candidates when matches are sparse, violating the fixed-memory rule.

## Current active queue — checkpoint 159 verification / 160

1. Confirm CMP #3739 and ESP32 Build #1638 on `93d858c8...`; checkpoint 159 is not GREEN until both are directly successful.
2. If GREEN, mark checkpoint 159 GREEN and start checkpoint 160 from current `cmp-protocol-v1` HEAD.
3. Continue only with measurable runtime/storage/flash candidates; do not force cosmetic refactors or helpers that increase linked flash for negligible benefit.
4. Prefer same-operation duplicate parsing/read/lookup elimination or bounded fixed-memory aggregate/tail techniques where historical integrity semantics remain intact.
5. Do not replace authoritative historical integrity scans with tail-only shortcuts.
6. Keep MaterialLedger `confirmUsage()` two-pass unless a future common writer lock spans preflight through atomic swap with equivalent recovery proof.
7. Keep separate-ledger validation for different integrity domains or distinct mutation phases.
8. Keep fixed-size RAM; no whole-file buffering or unbounded vectors.
9. Preserve HTTP preflight semantics, mutation-time TOCTOU validation, exact-spool provenance, deterministic recovery and all existing safety invariants.
10. No automatic production-data rotation/deletion/truncation and no premature DB/index migration.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff. Arduino owns SSR. ESP32/Web never controls SSR directly. Lost ACK never proves idle. Exact Material Request/item/spool/session/run provenance remains mandatory. Historical recovery stays deterministic. Restore stays operator-only and fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
