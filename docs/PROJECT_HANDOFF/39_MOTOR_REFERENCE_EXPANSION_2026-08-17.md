# Motor reference expansion checkpoint — 2026-08-17

Branch: `cmp-protocol-v1`

## Purpose

CoilMaster now maintains a large **read-only winding reference** that is intentionally separated from the verified working motor database.

The reference is built from `data/motor_catalog/**/*.source.json` into:

```text
firmware/esp32/web/reference/motor-reference.json
```

The generated index always carries:

```json
"reference_only": true
```

Unverified/reference records must never create a winding `coil_program`, trigger physical START, control SSR, or enter the production repair workflow automatically.

## Current coverage direction

The coverage goal is:

```text
MAXIMUM_REFERENCE_COVERAGE_WITH_VERIFIED_WORKING_DATABASE_SEPARATION
```

Active coverage currently includes:

- AIR / AIS-related reference data;
- 4A / 4AN;
- 4AM / 4AI / 4AIM-related repair/reference observations;
- 5A / 5AI / 5AM / 5AMX;
- A / AD / AL / AOL legacy families;
- A2 / AO2 / AOP2 / AOS2 / AOT2 / AOK2 / AOL2 / AOLS2;
- AO/AO2 multispeed machines;
- crane motors including stator + phase-rotor winding sets;
- repair-observed motors from mixed families;
- DC motors/generators (currently largely index-only where detailed source cards are images);
- single-phase motors with working/starting windings as separate winding sets;
- lift motors with separate speed windings;
- generators with multiple winding sets;
- high-voltage machines (index-only where detailed winding tables are not text-extractable);
- imported repair-observed motors;
- first real rectangular-wire observation inside repair data.

## Important source-handling rules

1. Raw source notation is authoritative for the reference layer.
2. Compound values such as `(10+10)3`, `21+21`, `5;3+5`, mixed wire diameters, and unusual source rows remain literal.
3. Source parallel winding branch count `a` is **not** CoilMaster `parallel_strands`.
4. A source `.n` / conductor-count column is stored separately when present and is not mapped automatically to `parallel_strands`.
5. Field-observation/repair data are never represented as manufacturer-verified data.
6. INDEX_ONLY records prove only that a source entry/model exists; they do not contain a usable winding program.
7. Suspect source values are preserved literally and flagged for review instead of being silently corrected.
8. Copied mirrors of the same technical table are not independent corroboration.

## Notable latest additions

Recent source packages include:

```text
data/motor_catalog/REPAIR_RECORDS/AIR_AIS_REPAIR_01.source.json
data/motor_catalog/5A/5A_6P_SUPPLEMENT_01.source.json
data/motor_catalog/AIR/AIR_180_355.source.json
```

The AIR/AIS repair package includes a real `4АМА100L2` observation and keeps a physically suspicious `АИР160S2` geometry row source-native with review status.

The 5A supplementary package preserves the separate 6/8-pole table semantics including the source conductor-count-per-turn column.

The AIR senior-frame package extends technical-reference coverage beyond the earlier 71–160 first pass through 180/200/225/250 and up to 335/355-frame entries.

## Current generated index checkpoint

After the latest completed source additions, the generated static reference reached:

```text
record_count = 855
reference_only = true
```

This count is a generated-reference count, **not** a count of VERIFIED production motors.

## Coverage status notes

`AIR` was changed from `FIRST_PASS_COMPLETE` back to `IN_PROGRESS` because coverage now extends beyond the original 71–160 first-pass boundary.

`4AM` is now `IN_PROGRESS` because real 4AM-family reference/repair observations are present, although a complete authoritative 4AM winding table has not yet been captured.

`6A` remains `PLANNED`: the source index confirms the family exists, but a sufficiently complete text-extractable winding table has not yet been captured. Do not synthesize 6A rows from type listings or general technical-characteristic tables.

`RECTANGULAR_LV` remains `PLANNED` as a dedicated series package. A real rectangular-wire repair observation already exists in the mixed repair layer, but the dedicated source table has not yet been captured in a safe complete form.

## Next reference priorities

Continue without weakening provenance rules:

1. complete additional AIR/AIS repair-observation rows from `air_ais_2_series.html`;
2. expand remaining 5A 6/8-pole rows from `5a_3_series.html`;
3. capture complete 4AM/4AI/4AIM winding tables when a reliable source becomes text-accessible;
4. capture 6A only from an actual winding-data table;
5. capture the dedicated low-voltage rectangular-wire table if/when its rows become extractable;
6. expand detailed DC records only when image cards can be read reliably — keep index-only entries otherwise;
7. keep static-reference regeneration and freshness checking mandatory after source changes.

## Safety boundary unchanged

Nothing in the winding reference changes CoilMaster production safety invariants:

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically deducts wire;
- wire write-off remains manual and tied to exact `spool_id + source_session_id + source_run_id`.
