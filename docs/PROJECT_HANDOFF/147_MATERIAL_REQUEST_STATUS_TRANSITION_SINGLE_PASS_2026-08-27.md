# 147 — Material Request status transition single-pass

Дата: **2026-08-27**  
Ветка: `cmp-protocol-v1`

## Change

`MaterialRequestStatusStore::transition()` no longer scans `/data/workshop/material-request-status.ndjson` once through `resolve()` and again through `nextTransitionId()`.

A private `analyzeStatus()` now performs one streamed status-journal pass that:

- preserves the current per-request lifecycle state (`DRAFT -> ISSUED -> PRICED -> CLOSED`);
- validates global monotonic `transition_id` ordering;
- validates the exact per-request transition chain;
- derives the next global transition id when called from mutation;
- fails closed on malformed records, invalid chains, overflow or id exhaustion.

Public `resolve(..., bool& found)` remains and delegates to the same analyzer without requesting a new id. The immutable request existence check still reads `/data/workshop/material-requests.ndjson` separately because it proves a different integrity domain.

## Commits

```text
9c5f4081148196cd6319ff44639c2008106492d3  analyzer declaration
 a0ec58c552bed883d289fa1e30160f365955a6e1  fused status-state + next-id scan
6c404070d80532e617174d4621d828cf16a094c0  mandatory status-store contract
```

## Verified

```text
ESP32 Build #1624      33036483178 / SUCCESS
CMP Protocol #3695     source commit / SUCCESS
CMP Protocol #3696     33036507740 / SUCCESS
```

CMP remains at 69 mandatory host audit steps; the existing Material Request status-store audit was extended instead of adding another workflow step.

## Preserved semantics

- missing immutable Material Request remains `true + found=false`;
- read/integrity failure remains `false`;
- default state remains DRAFT;
- only DRAFT->ISSUED, ISSUED->PRICED, PRICED->CLOSED are accepted;
- transition journal remains append-only;
- no automatic warehouse mutation, writeoff, START, resume or SSR action was introduced.
