# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 156; checkpoint 157 under CI

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
```

Production commits:

```text
1e831cde072f6c6152d10b7e71cb6a1e0f2a7b0e  checkpoint 152
a2f98cb377873d88d3fd103b6dfdfbabaf28ea65  checkpoint 154
415394d162de0f1c83e433cbbea3db94833b3162  checkpoint 155
78b41d38abdf89b9e72a02eea37edcd346c9610f  checkpoint 156
491fcf965fe573f91eb29bc99f6513017f3f5b1a  checkpoint 157 current production HEAD
```

Latest direct verification:

```text
CMP Tests #3725    33043013899 / SUCCESS  (checkpoint 155 production)
ESP32 Build #1630  33043013882 / SUCCESS  (checkpoint 155 production)
CMP Tests #3729    33043230389 / SUCCESS  (checkpoint 156 production)
ESP32 Build #1631  33043230391 / SUCCESS  (checkpoint 156 production)
CMP Tests #3730    33043322437 / SUCCESS  (checkpoint 156 handoff HEAD 61151ea...)
CMP Tests #3733    33043881805 / in_progress at last direct check (checkpoint 157 production)
ESP32 Build #1634  33043881890 / in_progress at last direct check (checkpoint 157 production)
```

CMP host audit remains 69 mandatory steps.

## Checkpoint 156

`MotorSimilarityWeb::handleLookup()` now fetches `coil_program` once and reuses it for required/validation/candidate construction. Existing HTTP behavior is unchanged: missing/empty -> `400 coil_program_required`; malformed -> `400 invalid_coil_program`; registry failure -> `500 similarity_lookup_failed`. CMP #3729 and ESP32 #1631 directly verify the production commit, so checkpoint 156 is GREEN.

## Checkpoint 157

`MaterialLedgerWeb` adjustment-history and usage-history handlers previously fetched optional `material_id`, `repair_id` and `limit` query strings once for empty checks and then fetched them again inside `parseUnsigned()`. The current path adds one shared `parseUnsignedValue(const String&)` implementation, fetches each optional query value once, and parses the already-held String. Cursor behavior remains strict: present-but-empty cursor is still invalid. Optional empty `repair_id` / `material_id` / `limit` keep their previous default/absent semantics. These are read-only history endpoints; material mutation, TOCTOU, WAL and RUN_WIRE paths are unchanged.

The larger repair-page/status candidate remains intentionally unchanged. Repairs may close out of `repair_id` order, so lockstep `repairs.ndjson` + `repair-status.ndjson` streaming is invalid. A request-wide status scan would require unbounded retained candidates when matches are sparse, violating the fixed-memory rule.

## Current active queue — checkpoint 157 verification / 158

1. Confirm CMP #3733 and ESP32 Build #1634 on `491fcf96...`; checkpoint 157 is not GREEN until both are directly successful.
2. If GREEN, mark checkpoint 157 GREEN and start checkpoint 158 from current `cmp-protocol-v1` HEAD.
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
