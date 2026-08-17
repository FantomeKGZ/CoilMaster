# 4A / 4А

Source packages for 4А-series motors. Preserve source provenance and variant-specific geometry and winding data.

## Current status

Raw technical-reference transcription now exists for practical frame sizes 50–250:

- `4A_50.source.json`
- `4A_56.source.json`
- `4A_63.source.json`
- `4A_71.source.json`
- `4A_80.source.json`
- `4A_90.source.json`
- `4A_100.source.json`
- `4A_112.source.json`
- `4A_132.source.json`
- `4A_160.source.json`
- `4A_180.source.json`
- `4A_200.source.json`
- `4A_225.source.json`
- `4A_250.source.json`

All raw records are exposed through the separate read-only winding reference. The generated static index is rebuilt from every `*.source.json` and must remain `reference_only=true`.

No 4A import-ready package exists yet.

Normalization boundaries:

- AIR/AIS `coil_program` derivation is intentionally not reused until the 4A winding-layout notation is independently documented.
- Source parallel winding branches `a` remain source-native metadata and are not mapped to CoilMaster `parallel_strands`.
- The 4A reference table exposes conditional stator-core lengths for different housing-length designations; raw files preserve those columns separately instead of forcing one scalar length.
- Compound pitch notation such as `5;3+5` remains reference-only until its 4A construction is proven.
- Parenthesized or compound N notation such as `(16+16)2`, `(14+14)3`, `21+21`, `(8+9)5` or `(4+5)8` remains reference-only until a dedicated 4A rule is proven.
- `4A` and `4AH` variants are retained as distinct model variants rather than merged.
- Source anomalies are preserved rather than silently repaired. The reference row for `4А71В6` exposes wire mass as `108`; `4A132M8` exposes `N=21.2`; source label `4AH2256` is also retained and flagged for review.
- Source rows marked with `*` in the frame-250 table use the source footnote voltage `380/660 V`; unstarred rows retain the table-wide `220/380 V` context.

## Next work

1. Verify the 4A winding notation/layout semantics against an independent technical table, winding diagram, or authoritative construction description.
2. Continue raw transcription with the next larger source-covered frame groups if present.
3. Create import-ready files only for variants covered by a documented deterministic mapping.
