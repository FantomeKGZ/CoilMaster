# Checkpoint 165 — Repair Delivery append single-pass preparation

Date: **2026-08-29**  
Branch: **`arduino-ru-lcd-experiment`**  
Production remains unchanged at **`cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c`**.

## Result — GREEN

`RepairDeliveryStore::append()` previously made two full authoritative passes over the growing `repair-deliveries.ndjson` journal before the append:

1. `resolveByRepair()` to prove that the repair had no existing delivery;
2. `nextDeliveryId()` to validate monotonic ids and derive the next `delivery_id`.

Both passes belonged to the same append-time mutation boundary and repeated the same NDJSON/schema/id-order work.

Checkpoint 165 replaces them with one private bounded-memory helper:

```cpp
prepareAppend(repairId, deliveryId)
```

The single pass now simultaneously:

- validates flat JSON records;
- validates strictly increasing non-zero `delivery_id`;
- validates non-zero `repair_id`;
- preserves the previous matching-repair record validation for `client_id`, `motor_id`, `delivered_at` and optional `comment`;
- rejects duplicate delivery provenance for the target repair;
- derives the next `delivery_id` from the final validated id;
- fails closed on id overflow or malformed journal data.

No whole-file buffer, persistent cache, index or database was introduced.

## Safety boundary retained

The Web layer still performs its separate preflight `resolveByRepair()` so an already delivered repair continues to receive the explicit HTTP 409 `repair_already_delivered` response.

The append itself still performs its own fresh authoritative `prepareAppend()` pass immediately before `FILE_APPEND`. Therefore the Web preflight is **not** reused across the mutation boundary and TOCTOU protection is not weakened.

This checkpoint only fuses the two redundant scans *inside the append mutation path*.

## Commits

```text
abd98e9bb16fd31e97369646668b909cbc39c6cb  declare single-pass append preparation
7403e3d088efed0a48a3be34a5ae0031cb0601b3  fuse delivery append journal scans
d32e56a5095278281fd6c43c31313d5816ced36d  lock single-pass delivery append contract
```

## Verified CI

Runtime commit `7403e3d...`:

```text
ESP32 Build #1761        run 33259525116 / SUCCESS
Arduino RU LCD #185      run 33259525099 / SUCCESS
```

Contract head `d32e56a...`:

```text
CMP Protocol Tests #3982 run 33259554454 / SUCCESS
```

## Unchanged invariants

- no automatic physical START/repeat/resume;
- Arduino remains the only SSR owner;
- RUN_COMPLETED remains evidence only;
- RUN_WIRE material writeoff remains explicit/manual with exact `spool_id + source_session_id + source_run_id`;
- delivery remains append-only and one-per-repair;
- repair delivery still requires a CLOSED repair;
- recovery/integrity scans remain fail-closed;
- confirmed history is never automatically deleted/truncated.

## Next

Continue the repeated-growing-journal audit only where multiple passes occur inside the same proof/mutation phase. Do not collapse the Web delivery preflight into append unless exact HTTP conflict semantics and mutation-time authoritative reread can both be preserved explicitly.
