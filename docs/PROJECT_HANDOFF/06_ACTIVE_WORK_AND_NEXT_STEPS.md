# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 157; checkpoint 158 under CI

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
```

Production commits:

```text
1e831cde072f6c6152d10b7e71cb6a1e0f2a7b0e  checkpoint 152
a2f98cb377873d88d3fd103b6dfdfbabaf28ea65  checkpoint 154
415394d162de0f1c83e433cbbea3db94833b3162  checkpoint 155
78b41d38abdf89b9e72a02eea37edcd346c9610f  checkpoint 156
491fcf965fe573f91eb29bc99f6513017f3f5b1a  checkpoint 157
0596ae9ff473503bd1d21aeda0c6c4da0f2ba0da  checkpoint 158 current production HEAD
```

Latest direct verification:

```text
CMP Tests #3729    33043230389 / SUCCESS  (checkpoint 156 production)
ESP32 Build #1631  33043230391 / SUCCESS  (checkpoint 156 production)
CMP Tests #3730    33043322437 / SUCCESS  (checkpoint 156 handoff)
CMP Tests #3731    33043838202 / SUCCESS
ESP32 Build #1632  33043838247 / SUCCESS
CMP Tests #3732    33043859931 / SUCCESS
ESP32 Build #1633  33043860032 / SUCCESS
CMP Tests #3733    33043881805 / SUCCESS  (checkpoint 157 production)
ESP32 Build #1634  33043881890 / SUCCESS  (checkpoint 157 production)
CMP Tests #3734    33043911946 / SUCCESS  (checkpoint 157 handoff HEAD be8a4a3...)
```

Checkpoint 158 verification started on `0596ae9f...`:

```text
CMP Tests #3735    33044295465 / in_progress at last direct check
ESP32 Build #1635  33044295425 / in_progress at last direct check
```

CMP host audit remains 69 mandatory steps.

## Checkpoint 157

Material adjustment-history and usage-history handlers now fetch each optional query value once and parse the already-held `String` through shared `parseUnsignedValue()`. Optional empty `repair_id` / `material_id` / `limit` preserve previous default semantics; present-but-empty cursor remains invalid. These are read-only paths. CMP #3733 and ESP32 #1634 directly verify the production commit, so checkpoint 157 is GREEN.

## Checkpoint 158

`MaterialLedgerWeb::handleAdjust()` previously fetched `add_quantity_milli` and `new_price_per_unit_minor` once to test for an empty optional value and then fetched them again inside `parseUnsigned()`. It now fetches each optional value once and parses that local `String` via the already-existing `parseUnsignedValue()` helper. HTTP behavior is preserved: absent/empty optional values remain zero/no-change, malformed non-empty values remain `400`, and the existing `no_adjustment_requested` rule remains unchanged. `adjustMaterial()`, WAL ordering, mutation-time revalidation, recovery, pricing semantics and RUN_WIRE paths are untouched.

The larger repair-page/status candidate remains intentionally unchanged. Repairs may close out of `repair_id` order, so lockstep `repairs.ndjson` + `repair-status.ndjson` streaming is invalid. A request-wide status scan would require unbounded retained candidates when matches are sparse, violating the fixed-memory rule.

## Current active queue — checkpoint 158 verification / 159

1. Confirm CMP #3735 and ESP32 Build #1635 on `0596ae9f...`; checkpoint 158 is not GREEN until both are directly successful.
2. If GREEN, mark checkpoint 158 GREEN and start checkpoint 159 from current `cmp-protocol-v1` HEAD.
3. Continue only with measurable runtime/storage/flash candidates; do not force cosmetic refactors.
4. Prefer same-operation duplicate parsing/read elimination or bounded fixed-memory aggregate/tail techniques where historical integrity semantics remain intact.
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
