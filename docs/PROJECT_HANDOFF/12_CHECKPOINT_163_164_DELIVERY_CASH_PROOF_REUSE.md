# Checkpoints 163–164 — repair delivery / cash proof reuse

Date: **2026-08-29**  
Branch: **`arduino-ru-lcd-experiment`**  
Production remains unchanged at **`cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c`**.

## Checkpoint 163 — Repair Delivery repair-status proof reuse — GREEN

`POST /api/repairs/delivery` already obtains an authoritative exact repair identity through:

```cpp
m_repairs.loadRepairIdentity(repairId, identity, repairFound)
```

After that successful proof, the CLOSED/open-state gate now uses the explicit status-only path:

```cpp
m_repairs.repairStatusIsOpen(repairId, repairOpen)
```

instead of calling generic `repairIsOpen()` and immediately rescanning `repairs.ndjson` for repair existence that was already proven in the same request.

Safety/integrity semantics remain unchanged:

- exact repair identity is still server-derived before delivery;
- missing repair still returns 404 fail-closed;
- delivery still requires the repair to be CLOSED;
- `RepairDeliveryStore::append()` keeps its own append-time uniqueness/integrity behavior;
- delivery remains independent of cash balance;
- no persistent cache/index/database or unbounded collection was introduced;
- generic repair lookup APIs remain self-validating for callers without prior proof.

Commits:

```text
ec6fbac31c1737b17643d0f93fb829742a4a8146  perf(repair): reuse delivery repair identity proof
bbe80dec4de608dfb89f1fa858d10d342ecf50fe  test(repair): guard delivery status proof reuse
```

Verification:

```text
runtime ec6fbac31c1737b17643d0f93fb829742a4a8146
ESP32 Build #1758        run 33258031527 / SUCCESS
Arduino RU LCD #182      run 33258031541 / SUCCESS

contract bbe80dec4de608dfb89f1fa858d10d342ecf50fe
CMP Protocol Tests #3976 run 33258064046 / SUCCESS
```

`CMP #3975` on the runtime commit failed only in a stale source-text contract after the proof-reuse refactor; runtime builds were SUCCESS. The contract commit corrected the expectation and #3976 passed.

## Checkpoint 164 — Cash payment repair-pricing proof reuse — GREEN

`CashPaymentWeb::handleCreate()` already obtains authoritative repair identity through:

```cpp
m_repairs.loadRepairIdentity(repairId, identity, found)
```

After that exact proof, pricing now uses:

```cpp
m_costing.loadKnownRepair(repairId, pricing)
```

instead of generic `m_costing.load(repairId, pricing)`, avoiding an immediate duplicate full `repairs.ndjson` validation while preserving all costing-ledger checks performed by the known-repair path.

Unchanged safety/integrity boundaries:

- generic `RepairCosting::load()` remains self-validating for callers without prior repair proof;
- payment currency must still match authoritative repair pricing;
- correction target provenance remains checked with `eventBelongsToRepair()`;
- SUBTRACT still validates current paid total before accepting the correction;
- `CashPaymentStore::append()` retains its mutation-time authoritative scan/checks;
- CashPayment correction fusion remains NO-CHANGE because combining the correction target lookup/totals path would duplicate validation logic and risks changing fail-closed semantics;
- no cache/index/database, whole-file buffering or automatic history mutation was added.

Runtime commit:

```text
60a9dbd0542d800f5ea36d727b139facf77532b3  perf(cash): reuse repair identity pricing proof
```

Runtime compilation evidence:

```text
ESP32 Build #1759        run 33258146828 / SUCCESS
Arduino RU LCD #183      run 33258146657 / SUCCESS
```

`CMP #3977` (`33258146690`) failed only at `Tests/Web/check_crm_backup_integrity.js` because that older static contract still required the pre-optimization literal:

```text
m_costing.load(repairId, pricing)
```

All CMake/CTest runtime tests and all other Web/static contract steps passed in #3977.

The stale CRM contract was corrected in:

```text
490dde39c4fae1aa1241e7c0b79d0cdc46fe7f41  test(web): align CRM audit with cash pricing proof reuse
```

The updated contract now requires `loadKnownRepair(repairId, pricing)` and rejects reintroduction of the duplicate generic `load(repairId, pricing)` call after exact repair proof.

Final exact verification:

```text
CMP Protocol Tests #3978 run 33259276639 / SUCCESS
head 490dde39c4fae1aa1241e7c0b79d0cdc46fe7f41
```

No C++ runtime source changed in `490dde39`; the latest runtime compile evidence for checkpoint 164 remains ESP32 #1759 and Arduino RU LCD #183 on `60a9dbd...`.

## Adjacent audit result

`RepairRegistryWeb` finalization/close paths were rechecked after #3978:

- finalization already uses `RepairFinalizationGuard::checkKnownRepair()` after authoritative `repairIsOpen()` proof (checkpoint 162);
- close still retains `m_registry.closeRepair(...)` as the later mutation-time authoritative reread;
- therefore no additional optimization is safe/necessary there and the path remains **NO-CHANGE**.

## Next

Continue only in `arduino-ru-lcd-experiment` with repo-reviewable repeated-growing-journal scans where an existing authoritative proof can be reused in the same request. Do not remove mutation/recovery/TOCTOU rereads, do not weaken exact RUN_WIRE provenance, and do not introduce persistent caches/indexes/DB or unbounded NDJSON buffering.
