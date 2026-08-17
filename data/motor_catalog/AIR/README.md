# AIR / АИР

Source packages for АИР-series motors.

First-pass scope: frame sizes 71, 80, 90, 100, 112, 132, and 160 with separate variants for poles, voltage, geometry, connection, and conductor construction where source data differs.

## Current status

Raw source transcription is complete for:

- `AIR_71.source.json`
- `AIR_80.source.json`
- `AIR_90.source.json`
- `AIR_100.source.json`
- `AIR_112.source.json`
- `AIR_132.source.json`
- `AIR_160.source.json`

Import-ready normalized packages now exist for every source row that satisfies `../NORMALIZATION_RULES.md`:

- `AIR_71.json` — 6 simple single-layer reference variants normalized; compound 8-pole and ambiguous repair rows remain staging-only.
- `AIR_80.json` — 6 simple 2/4/6-pole reference variants normalized; `5;3+5` 8-pole and ambiguous repair rows remain staging-only.
- `AIR_90.json` — 4 simple reference variants normalized; the compound 18-slot repair variant remains staging-only.
- `AIR_100.json` — 6 simple reference variants normalized, including explicit two-wire-in-hand `×2` records.
- `AIR_112.json` — 6 simple reference variants normalized; conflicting 30-slot repair variants remain staging-only.
- `AIR_132.json` — 7 simple reference variants normalized; explicit wire-in-hand counts are preserved independently from source parallel winding branches.
- `AIR_160.json` — 4 simple 6/8-pole reference variants normalized; 2/4-pole records with `(15+16)2`, `(12+13)2`, `(6+7)4` and compound repair notation remain staging-only.

The first-pass AIR 71–160 normalization is therefore complete for all records covered by the currently proven rule set.

Normalization rules document:

- deterministic mapping of simple single-layer concentric pitch lists to CoilMaster `coil_program`;
- explicit wire-in-hand notation as the only current source for `parallel_strands`;
- source parallel winding branches `a` are never mapped to `parallel_strands`;
- unsupported compound/two-layer notation remains staging-only;
- fractional source dimensions are not rounded into the importer's integer-only numeric fields and are retained losslessly in source data/comments.

Materially different repair observations remain separate physical variants. Source inconsistencies are preserved and flagged rather than silently corrected; for example, a repair observation for `АИР160S2` contains geometrically suspect Da/Di values.

## Next work

1. Keep complex AIR repair/two-layer notation staged until a separate authoritative mapping is documented.
2. Start the next catalogue family rather than weakening AIR validation rules merely to increase record count.
3. Preferred next family: `4A`, then `AO2`; `AIS` should be split out only where source rows are demonstrably AIS-specific rather than aliases paired with AIR rows.
