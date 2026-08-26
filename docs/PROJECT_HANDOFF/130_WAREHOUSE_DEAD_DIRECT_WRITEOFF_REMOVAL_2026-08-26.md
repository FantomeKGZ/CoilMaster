# Checkpoint 130 — dead direct warehouse writeoff removal

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Status

**GREEN (software / CI).**

## Removed

The obsolete direct Store mutation entrypoints are fully deleted:

```text
WarehouseStore::confirmSpoolWriteOff(...)
WarehouseStore::confirmKgFirstWriteOff(...)
SpoolWriteOffResult
```

They had no production callers after the public legacy POST was disabled and checkpoint 128 made them non-public.

## Retained deliberately

Historical reboot reconciliation still needs the old journal shape and append helpers:

```text
ConfirmedSpoolWriteOff        private recovery-only shape
appendWriteOffRecord(...)     closes historical LEGACY_SPOOL PENDING
appendKgFirstWriteOffRecord(...) historical/managed KG_FIRST evidence
recoverPendingWriteOff()      deterministic BEFORE/AFTER reconciliation
```

Current production mutation remains only:

```text
explicit POST /api/material-requests/warehouse
-> RunWireIssueCoordinator
-> prepareManagedRunWireWriteOff
-> applyManagedRunWireSpoolWeight
-> confirmManagedRunWireWriteOff
```

All current exact spool/session/run/completion/duplicate checks remain in the atomic RUN_WIRE coordinator. Historical recovery does not create a new operation; it only resolves already-durable PENDING state from exact persisted spool weight.

## Source / contract commits

```text
e9ebd56a0317a7aecf87d4e6fd49e5a3433c22fd  remove dead Store declarations/result type
2a0bde9954edbfb712336c38c677d70d405b0332  delete dead direct implementations
0cc7c1232cdfd38ca8423ee159d5f2edd6f58f64  initial dead-entrypoint contract
175cf3a6ea483921216db4daab155d143174b9fd  RUN_WIRE contract alignment
95da1c014089d5ad05485819886158deac489cb8  release safety alignment
694fca7b8f2dd40d9adbdf5441902646a1af7c2d  final acceptance alignment
75e9d68847ed4e5953c1fddc18faf6a5c54a4b06  managed KG-first contract alignment
6f355a7aa5b2071477b3a9bd8ac387d96abf0e13  recovery/fault contract alignment
```

## Verified CI

```text
ESP32 Build #1574
run 32963503796
SUCCESS

CMP Protocol Tests #3563
run 32964152182
SUCCESS
```

All 68 host-test audit steps in #3563 completed SUCCESS.

## Safety preserved

- `RUN_COMPLETED` is evidence only;
- no automatic writeoff;
- no automatic physical START or resume;
- public legacy writeoff POST remains HTTP 410/no-write;
- deterministic historical recovery remains available;
- no journal/data migration;
- bounded history/report compatibility remains unchanged.

## NEXT

Continue warehouse/software cleanup only where compile + contracts prove a helper is unused. Do not remove recovery append helpers or historical codecs while persisted legacy PENDING/history can exist. Prefer existing direct immutable RUN_WIRE movement provenance and avoid duplicate full-log scans.
