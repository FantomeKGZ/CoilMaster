# CoilMaster — transactional repair intake integration

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN**

Этот checkpoint закрывает Phase-A блок автоматического immutable `AS_RECEIVED` capture при `POST /api/repairs`.

## Implementation

Основной integrated commit:

```text
92523c7c6f4c8af8c71a63c4178a4b1e41953f19
feat(crm): make repair intake snapshot transactional
```

Добавлено/подключено:

```text
CM_RepairIntakeCoordinator
CM_RepairIntakePendingStore
RepairRegistryWeb -> RepairIntakeCoordinator
```

`POST /api/repairs` больше не вызывает `m_registry.addRepair()` напрямую.

## Transaction sequence

```text
validate source
-> compute expected repair_id
-> prepare exact winding/legacy intake source
-> persist pending marker
-> append repair
-> require actual repair_id == expected repair_id
-> append immutable AS_RECEIVED snapshot
-> verify exact snapshot provenance
-> clear pending marker
-> return HTTP 201
```

## Power-loss recovery

При startup `RepairIntakeCoordinator::begin()` вызывает recovery до normal create-repair operation.

Cases:

1. pending exists, repair absent, snapshot absent, expected id unused -> pending is uncommitted and may be cleared;
2. repair exists, snapshot missing -> rebuild exact snapshot from pending/source and verify it;
3. repair + exact snapshot exist -> clear stale pending marker;
4. snapshot without expected repair or any identity mismatch/ambiguity -> fail closed;
5. active/invalid pending marker blocks new repair creation instead of guessing.

## Legacy compatibility

If a motor has a versioned winding, pending stores exact `sourceWindingVersionId` and recovery reconstructs through exact version-id lookup.

If no winding version exists, source is explicit `LEGACY_MOTOR` and snapshot is built from legacy motor master fields.

No destructive rewrite of existing `motors.ndjson` / `repairs.ndjson`.

## Regression

Permanent regression:

```text
Tests/Web/check_repair_intake_transaction_integration.js
```

CI step:

```text
Audit transactional repair intake integration
```

One-shot workflow used only to apply the guarded large-file patch was removed afterwards.

## Verification

Current verification HEAD:

```text
0a45c7c23ee62149d21b13040fbb485b05f42d10
```

Runs:

```text
CMP Protocol Tests #3182 / run 32851184680 / SUCCESS
ESP32 Build #1460 / run 32851184075 / SUCCESS
```

The CMP run includes the permanent transactional repair intake regression.

## Next mandatory step

Before new CRM stores become release-critical:

1. add winding version + AS_RECEIVED + intake transaction artifacts to backup/restore whitelist where appropriate;
2. extend integrity audit for their schemas/cross-references;
3. only then start Material Request persistence from checkpoint 101.

## Safety

Unchanged:

- no automatic physical START;
- Arduino owns SSR;
- `RUN_COMPLETED` never auto-deducts material;
- cancellation/operator abort never erases immutable evidence;
- restore is operator-only, transactional and fail-closed;
- no automatic production-data deletion/truncation.
