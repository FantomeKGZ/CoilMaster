# Checkpoint 122 — atomic RUN_WIRE operator UI

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Status

**GREEN.** Desktop/mobile manual wire writeoff now uses the checkpoint-121 atomic Material Request RUN_WIRE transaction instead of the legacy direct warehouse writeoff POST.

## Production operator flow

```text
RUN_COMPLETED
  -> operator opens writeoff
  -> exact uncovered source_session_id + source_run_id
  -> immutable JobSpoolSelection
  -> ACTIVE exact spool
  -> exact spool-material bridge
  -> explicit DRAFT/ISSUED Material Request selection
  -> explicit operator submit
  -> POST /api/material-requests/warehouse
       confirmed=true
       movement_kind=ISSUE
       source_kind=RUN_WIRE
       unit=KG
       material_request_id=<explicit selection>
       warehouse_item_id=<bridge item>
       source_session_id=<exact session>
       source_run_id=<exact run>
       spool_id=<exact immutable spool>
       material_class=CU|AL
       wire_diameter_hundredths_mm=<exact diameter>
       quantity_milli_units=<actual consumed grams>
```

`RUN_COMPLETED` remains read-only evidence and never triggers stock mutation automatically.

## UI changes

- desktop and mobile use the same shared operator controller;
- both surfaces expose mandatory Material Request selection;
- only `DRAFT` / `ISSUED` requests belonging to the exact repair are offered;
- the UI resolves exact `spool_id -> warehouse_item_id` through the read-only spool-material bridge endpoint;
- bridge CU/AL + exact diameter must agree with the ACTIVE physical spool;
- submit remains disabled until repair, completed run, immutable selection, ACTIVE spool, bridge and Material Request are all resolved;
- quantity input remains exact kg with at most three decimal digits and is converted to integer consumed grams;
- successful response must return a valid movement id and the same exact spool id before the UI advances to the next uncovered run;
- legacy `/api/warehouse/write-offs` remains GET-only in this operator controller for history/coverage. The production UI no longer POSTs directly to it.

## Compatibility boundary

The legacy/direct warehouse writeoff endpoint remains implemented for compatibility and historical/fault recovery contracts. Checkpoint 122 does **not** delete or weaken it. Operator production flow is now routed through the higher-level atomic RUN_WIRE transaction.

Existing standard warehouse CONFIRMED evidence emitted by checkpoint 121 remains the source consumed by writeoff history, costing and strict finalization coverage. This avoids double-accounting while preserving old read/report compatibility.

## Contract coverage

`Tests/Web/check_kg_first_material_contracts.js` now enforces:

- mandatory Material Request controls on desktop/mobile;
- exact atomic RUN_WIRE POST fields;
- bridge + request/status lookups;
- no production UI legacy/direct POST;
- no UNALLOCATED fallback;
- no automatic RUN_COMPLETED deduction.

`Tests/Web/check_writeoff_fault_contracts.js` now separates:

- legacy/direct backend fail-closed recovery compatibility; and
- production operator atomic RUN_WIRE frontend fault semantics.

The UI advances only after a successful atomic response with exact movement/spool identity.

## Commits

```text
81aa293470626436c09fd444eb684bb0b42a0fdb  feat(web): add material request choice to mobile writeoff
5c28fadd4a3d1ef8de272f677e2b2f53bfc77794  feat(web): route operator wire writeoff through atomic RUN_WIRE
10528b23336bebe30208a56e085d3d77aeb19af9 test(web): enforce atomic RUN_WIRE operator writeoff
f8d25c1b5fb04bddbd0c2b93fca704f14a7b565f test(web): align writeoff fault contract with atomic RUN_WIRE
```

## Verified CI evidence

```text
ESP32 Build #1555  32954324723 / SUCCESS   (mobile UI source)
ESP32 Build #1556  32954467677 / SUCCESS   (shared atomic controller source)
Reference Legacy #101 32954324914 / SUCCESS
Reference Legacy #102 32954467670 / SUCCESS
CMP Tests #3489    32954794059 / SUCCESS   (final contract state)
```

CMP #3488 failed only because `check_writeoff_fault_contracts.js` still required the retired legacy operator POST string. The production implementation itself had already passed the kg-first contract; the stale fault test was updated to the intended atomic boundary and #3489 is GREEN.

## Next

Audit downstream read/report provenance for the new operator path:

1. expose or verify exact `material_request_id + transaction_ref + warehouse_item_id` where operationally useful;
2. prove costing/finalization/report consumers do not double-count MaterialLedger usage against the standard physical warehouse movement;
3. add duplicate/double-accounting contract coverage between compatibility direct writeoff and atomic RUN_WIRE;
4. keep hardware E2E deferred until this software accounting/report chain stabilizes.
