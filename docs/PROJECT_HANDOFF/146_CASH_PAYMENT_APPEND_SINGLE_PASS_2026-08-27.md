# 146 — Cash payment append single-pass

Дата: **2026-08-27**  
Ветка: `cmp-protocol-v1`

## Change

`CashPaymentStore::append()` no longer scans `/data/workshop/repair-payments.ndjson` once through `eventExists(corrects_event_id)` and again through `nextEventId()`.

A private `analyzeAppendState()` now performs one streamed mutation-time pass that:

- validates monotonic non-zero `cash_event_id` values;
- finds the optional correction reference;
- derives the next event id;
- fails closed on malformed NDJSON or uint32 id exhaustion.

The public bool-only `eventExists()` API was removed. `eventBelongsToRepair(..., bool& found)` remains public and unchanged for the richer Web preflight that verifies exact repair/client provenance before mutation.

No destructive payment operation was added. Cash remains append-only and separated from machine/SSR/warehouse control.

## Commits

```text
58ed90a365962770af8ea2a8a7ec57f11372849c  header/API narrowing
 e15222e299ed4736a66d577175cf4e381e29747a  fused mutation-time scan
2163986a92a60ae177f2ee68000fa5e2f008236d  single-pass contract
 a96953ab4a51370dcb6402580def7f2de0256011  mandatory CMP workflow step
```

## Verified

```text
ESP32 Build #1622      33035968880 / SUCCESS
CMP Protocol #3687     33035968846 / SUCCESS
CMP Protocol #3689     33036075195 / SUCCESS
```

`#3689` includes the new mandatory `Audit cash payment single-pass contracts` step. Host audit count is now 69 and all steps passed.

## Preserved semantics

- Web correction ownership preflight remains explicit via `eventBelongsToRepair`.
- Mutation-time TOCTOU revalidation remains present.
- payment journal remains append-only.
- no automatic writeoff, START, SSR action, warehouse mutation or repair-state mutation is introduced.
