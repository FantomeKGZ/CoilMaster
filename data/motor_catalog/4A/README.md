# 4A / 4А

Source packages for 4А-series motors. Preserve source provenance and variant-specific geometry and winding data.

## Current status

- `4A_50.source.json` — raw technical-reference transcription created for 4А50А2, 4А50В2, 4А50А4, and 4А50В4.
- No 4A import-ready package exists yet.
- AIR/AIS `coil_program` derivation is intentionally not reused until the 4A winding-layout notation is independently documented.
- Source parallel winding branches `a` remain source-native metadata and are not mapped to CoilMaster `parallel_strands`.
- The 4A reference table exposes conditional stator-core lengths for different housing-length designations; raw files preserve those columns separately instead of forcing one scalar length.

## Next work

1. Verify the 4A winding notation/layout semantics against a technical table or winding diagram.
2. Continue raw transcription through the next practical frame sizes.
3. Create import-ready files only for variants covered by a documented deterministic mapping.
