# AIR / АИР

Source packages for АИР-series motors.

Planned first pass: frame sizes 71, 80, 90, 100, 112, 132, and 160 with separate variants for poles, voltage, geometry, and connection where source data differs.

## Current status

Raw source transcription is complete for:

- `AIR_71.source.json`
- `AIR_80.source.json`
- `AIR_90.source.json`
- `AIR_100.source.json`
- `AIR_112.source.json`
- `AIR_132.source.json`
- `AIR_160.source.json`

Import-ready normalized packages now exist for the source rows that satisfy `../NORMALIZATION_RULES.md`:

- `AIR_71.json` — simple single-layer reference variants normalized; compound 8-pole and ambiguous repair rows remain staging-only.
- `AIR_80.json` — simple 2/4/6-pole reference variants normalized; `5;3+5` 8-pole and ambiguous repair rows remain staging-only.
- `AIR_90.json` — all four simple reference variants normalized; the compound 18-slot repair variant remains staging-only.
- `AIR_100.json` — six simple reference variants normalized, including explicit two-wire-in-hand `×2` records.
- `AIR_112.json` — six simple reference variants normalized; conflicting 30-slot repair variants remain staging-only.

Normalization rules now document:

- deterministic mapping of simple single-layer concentric pitch lists to CoilMaster `coil_program`;
- explicit wire-in-hand notation as the only current source for `parallel_strands`;
- source parallel branches are never mapped to `parallel_strands`;
- unsupported compound/two-layer notation remains staging-only;
- fractional source dimensions are not rounded into the importer's integer-only numeric fields and are retained losslessly in source data/comments.

Materially different repair observations remain separate physical variants. Source inconsistencies are preserved and flagged rather than silently corrected; for example, a repair observation for `АИР160S2` contains geometrically suspect Da/Di values.

Next normalization targets: `AIR_132.json`, then `AIR_160.json`. Complex repair/two-layer notation remains a separate follow-up rule set.
