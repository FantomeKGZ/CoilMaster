# 4A / 4А

Source packages for 4А-series motors. Preserve source provenance and variant-specific geometry and winding data.

## Current status

Raw technical-reference transcription now exists for practical frame sizes 50–100:

- `4A_50.source.json`
- `4A_56.source.json`
- `4A_63.source.json`
- `4A_71.source.json`
- `4A_80.source.json`
- `4A_90.source.json`
- `4A_100.source.json`

No 4A import-ready package exists yet.

Normalization boundaries:

- AIR/AIS `coil_program` derivation is intentionally not reused until the 4A winding-layout notation is independently documented.
- Source parallel winding branches `a` remain source-native metadata and are not mapped to CoilMaster `parallel_strands`.
- The 4A reference table exposes conditional stator-core lengths for different housing-length designations; raw files preserve those columns separately instead of forcing one scalar length.
- Compound pitch notation such as `5;3+5` remains staging-only until its 4A construction is proven.
- Source anomalies are preserved rather than silently repaired. The reference row for `4А71В6` currently exposes wire mass as `108`, which is retained as suspect source text pending independent verification.

## Next work

1. Verify the 4A winding notation/layout semantics against an independent technical table, winding diagram, or authoritative construction description.
2. Continue raw transcription into larger frame sizes beginning with the next source-covered groups.
3. Create import-ready files only for variants covered by a documented deterministic mapping.
