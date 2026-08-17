# Motor reference expansion checkpoint — 2026-08-17

Branch: `cmp-protocol-v1`

## Purpose

CoilMaster maintains a large **read-only winding reference** that is intentionally separated from the verified working motor database.

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

Active coverage includes:

- AIR / AIS-related reference data;
- 4A / 4AN;
- 4AM / 4AI / 4AIM-related repair/reference observations;
- 5A / 5AI / 5AM / 5AMX;
- A / AD / AL / AOL legacy families;
- A2 / AO2 / AOP2 / AOS2 / AOT2 / AOK2 / AOL2 / AOLS2;
- AO/AO2 and mixed multispeed machines;
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
9. Source aliases such as AIR/AIS paired designations are preserved in the generated static index as `aliases`; they remain aliases of one source observation and are not duplicated as independent records.

## Notable latest additions

Recent source packages include:

```text
data/motor_catalog/REPAIR_RECORDS/AIR_AIS_REPAIR_01.source.json
data/motor_catalog/5A/5A_6P_SUPPLEMENT_01.source.json
data/motor_catalog/AIR/AIR_180_355.source.json
data/motor_catalog/5A/5A_6P_8P_SUPPLEMENT_02.source.json
data/motor_catalog/5A/5A_8P_SUPPLEMENT_03.source.json
data/motor_catalog/5A/5A_6P_12P_SUPPLEMENT_04.source.json
data/motor_catalog/4AM/4AM_MULTISPEED_01.source.json
data/motor_catalog/AIR/AIR_AIS_REPAIR_02.source.json
data/motor_catalog/AO_MULTI/MIXED_MULTISPEED_03.source.json
data/motor_catalog/AO_MULTI/MIXED_MULTISPEED_04.source.json
data/motor_catalog/AO_MULTI/MIXED_MULTISPEED_05.source.json
data/motor_catalog/AO_MULTI/MIXED_MULTISPEED_06.source.json
```

The AIR/AIS repair layer includes the full currently text-extractable repair-observation tail from `air_ais_2_series.html`, preserving physically distinct variants separately. This includes multiple `АИР160S2` and `АИР100S4` implementations, `АИР180/200/250` repair observations, `АИМ90L4`, and `2АИ90L4ПАУ3`.

The AIR/AIS technical-reference rows preserve source aliases such as `АИР71А2` / `АИС80А2`. The static generator now carries `aliases_in_source` into `aliases`, and both desktop and mobile winding-reference search include those aliases. This makes AIS designations searchable without duplicating the underlying source observation.

The 5A supplementary packages preserve the separate 6/8/12-pole table semantics including the source conductor-count-per-turn column `.n`. The latest 6P/12P supplement adds the missing 250/280-frame 6-pole rows plus `5AM315S12` and `5AM315M12`. The senior 8-pole supplement covers the text-extractable 250/280/315-frame 8-pole tail.

The AIR senior-frame package extends technical-reference coverage beyond the earlier 71–160 first pass through 180/200/225/250 and up to 335/355-frame entries.

The 4AM layer includes multispeed `4АМА100L4/2У3`, `4АМ180М8/4У2`, `4АМ132S6/4У4`, and a source-observed winding-machine motor based on `4АМ100L8` with two independent speed windings.

The mixed multispeed layer now covers the currently text-extractable `mnogo_skor_series.html` rows, including two-, three- and four-speed machines. The latest completion package adds `АИР80А4/2У3` and preserves `ДСХН I-42/8-642` as four source speed points without inventing separate winding sets from incomplete row layout.

## Current generated index checkpoint

After the latest completed source additions and alias-preservation rebuild, the generated static reference reached:

```text
record_count = 934
reference_only = true
```

This count is a generated-reference count, **not** a count of VERIFIED production motors. Alias support itself does not create duplicate records.

## Coverage status notes

`AIR` is `IN_PROGRESS` because coverage now extends well beyond the original 71–160 first-pass boundary and includes both serial reference rows and separate repair observations.

`AIS` is now `IN_PROGRESS`: AIS designations are explicitly present as aliases in the AIR/AIS source table and are searchable in the generated static reference without duplicating source records.

`4AM` is `IN_PROGRESS` because real 4AM-family repair/reference and multispeed observations are present, although a complete authoritative 4AM winding table has not yet been captured.

`5A` remains `IN_PROGRESS`; ordinary repair/reference rows plus separate 6/8/12-pole technical-reference semantics are preserved without collapsing `.n` into parallel branches.

`6A` remains `PLANNED`: the source index confirms the family exists, but direct section searches still do not expose a sufficiently complete text-extractable winding table. Do not synthesize 6A rows from type listings or general technical-characteristic tables.

`RECTANGULAR_LV` remains `PLANNED` as a dedicated series package. A real rectangular-wire repair observation already exists in the mixed repair layer, but the dedicated source table has not yet been captured in a safe complete form.

The current crane package already covers the text-extractable repair and handbook rows surfaced from `kran_series.html`; do not duplicate them when search results repeat the same table.

The current `import_3_series.html` text-extractable table is covered by `IMPORT_03` + `IMPORT_04`, including anonymous stator and KARCHER rows; do not duplicate them when search results repeat the same table.

## Next reference priorities

Continue without weakening provenance rules:

1. search for additional non-duplicate repair-observation rows outside the now-covered mixed multispeed text output;
2. capture complete 4AM/4AI/4AIM winding tables when a reliable source becomes text-accessible;
3. capture 6A only from an actual winding-data table containing `N`, wire and pitch;
4. capture the dedicated low-voltage rectangular-wire table if/when its rows become extractable;
5. expand detailed DC records only when image cards can be read reliably — keep index-only entries otherwise;
6. keep alias-preservation in the generated reference and desktop/mobile search;
7. keep static-reference regeneration and freshness checking mandatory after source changes.

## Safety boundary unchanged

Nothing in the winding reference changes CoilMaster production safety invariants:

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically deducts wire;
- wire write-off remains manual and tied to exact `spool_id + source_session_id + source_run_id`.
