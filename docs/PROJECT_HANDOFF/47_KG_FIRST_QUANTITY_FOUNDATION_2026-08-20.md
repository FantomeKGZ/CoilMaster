# KG-first quantity foundation — 2026-08-20

## Status

First code increment after the storage/API audit is committed on `cmp-protocol-v1`.

## Added

### Exact kg parser

`firmware/esp32/src/CM_KgQuantity.h`

The kg-first accounting boundary now has a floating-point-free parser:

- accepts positive decimal kilograms;
- accepts at most 3 fractional decimal digits because warehouse stock resolution is 1 gram;
- converts deterministically to integral grams;
- rejects zero, malformed values, repeated decimal separators and overflow;
- provides canonical kg rendering from grams;
- does not use `float`, `double`, `atof`, `strtod` or `String::toFloat()`.

This keeps operator-facing kg authoritative while preserving exact integer accounting compatible with current `mass_g` and `price_per_kg` arithmetic.

### Regression contract

`Tests/Web/check_kg_first_material_contracts.js`

The contract protects:

- exact decimal-to-gram conversion foundation;
- no floating-point parser at the accounting boundary;
- exact `source_session_id + source_run_id` provenance remains present in the existing manual write-off flow;
- completed RUN coverage remains the finalization anchor;
- no automatic write-off hook is introduced by the migration.

## Commits

```text
5d749fe48596947a2ec161781c2c848325dc0eaa  docs: audit kg-first warehouse invariants
ec728224d14032d05515bf9a45b334911f70483b  feat: add exact kg quantity parser
5bd57afa7133b7e58aabbf53f7b3dcd19935037e  test: guard kg-first quantity contract
```

## Production behavior

No production deduction behavior has changed yet.

Legacy exact-spool manual write-off remains the only active write-off path until the coordinated dual-shape journal migration is complete. This is intentional: exposing spool-less POST behavior before updating movement integrity/history/recovery/coverage would make storage validation inconsistent.

`RUN_COMPLETED` still never performs an automatic deduction.

## Verification

GitHub reports no status checks and no workflow runs for HEAD `5bd57afa7133b7e58aabbf53f7b3dcd19935037e` at this checkpoint.

Therefore:

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

## Exact next step

Implement the coordinated kg-first manual-consumption journal shape across:

1. `CM_WarehouseStore.h`;
2. `CM_WarehouseWriteOff.cpp`;
3. `CM_WarehouseWriteOffWeb.cpp`;
4. `CM_WarehouseMovementIntegrityAudit.cpp`;
5. `CM_WarehouseWriteOffHistory.cpp`;
6. `CM_WarehouseWriteOffRecovery.cpp`;
7. `CM_WireWriteOffCoverageAudit.cpp`.

Required semantics:

- operator `quantity_kg` -> exact grams through `KgQuantity`;
- exact source session/run required and unique;
- immutable conductor/material snapshot;
- optional `spool_id` only for the new kg-first mode;
- with spool: guarded exact stock mutation;
- without spool: append manual consumption, mutate no spool;
- preserve all legacy exact-spool records and checks;
- no automatic deduction on RUN completion.
