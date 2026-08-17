# AIR / АИР

Source packages for АИР-series motors.

Planned first pass: frame sizes 71, 80, 90, 100, 112, 132, and 160 with separate variants for poles, voltage, geometry, and connection where source data differs.

## Current status

- `AIR_71.source.json` — raw technical-source transcription created; not import-ready.
- `AIR_80.source.json` — raw technical-source transcription created; not import-ready.
- `AIR_90.source.json` — raw technical-source transcription created; not import-ready.
- AIR normalization is blocked on an authoritative mapping from source turns/conductors-in-slot data to CoilMaster's ordered `coil_program` sequence.
- Source winding parallel branches remain source-native metadata and are not mapped to `parallel_strands`.
- Materially different repair observations are retained as separate physical variants instead of being merged with reference-table variants.

Next source package: `AIR_100.source.json`.
