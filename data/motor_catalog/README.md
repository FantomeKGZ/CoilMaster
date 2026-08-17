# CoilMaster source motor catalogue

This directory contains curated source packages for the CoilMaster motor and winding database.

It is a repository-side source catalogue. Files here are reviewed and normalized before they are imported into a device through the existing motor import UI.

## Layout

- one directory per motor series or manufacturer family;
- one source/staging JSON package per frame-size/model group while technical semantics are still being verified;
- one import-ready JSON package per frame-size/model group only after all required CoilMaster fields are supported by the source or a documented calculation;
- `catalog.json` is the catalogue index and population-status registry;
- import packages must follow `docs/MOTOR_IMPORT_FORMAT.md`;
- package size must remain within the importer limit of 1–50 records.

File naming contract:

- `*.source.json` — raw/source-native transcription; **not import-ready** and must never be submitted to the device motor importer;
- `*.json` without `.source` — import-ready package and must strictly follow `docs/MOTOR_IMPORT_FORMAT.md`.

Recommended naming examples:

- `AIR/AIR_71.source.json` — source transcription while `coil_program` mapping is unresolved;
- `AIR/AIR_71.json` — future import-ready package after normalization;
- `AIR/AIR_80.json`
- `4A/4A_100.json`
- `ABB/M3AA_160.json`

## Data rules

1. Never invent winding values to make a record complete.
2. Never manufacture a required importer field just to promote a `.source.json` file to import-ready status.
3. Preserve the exact source title and source URL when available.
4. Do not treat copies of the same table on different websites as independent corroboration.
5. Use `VERIFIED` only for manufacturer documentation or a physically verified original winding record.
6. Use `CORROBORATED` only when at least two genuinely independent credible sources agree.
7. Use `CALCULATED` when an essential winding value was derived rather than copied from the cited source.
8. Keep uncertain single-source material `UNVERIFIED` until it is checked.
9. Different voltage, stator core length, slot count, pole count, or connection is a separate motor variant unless equivalence is proven.
10. Source winding parallel branches are not the same thing as CoilMaster `parallel_strands`; never map them automatically.
11. A source column described as turns in slot or conductors in slot is not automatically equivalent to CoilMaster `turns_per_coil` or `coil_program`.
12. `coil_program` represents the ordered turns of individual coils/program steps used by CoilMaster; source data must support that ordered sequence before an import-ready package is created.

## Initial series plan

The catalogue is populated in controlled passes. The first priority is AIR/AIS, then Soviet/common CIS series, crane motors, and finally imported manufacturer families.

Initial groups tracked by `catalog.json`:

- AIR
- AIS
- 4A
- 4AM
- A2
- AO2
- AO
- AOL
- MT
- MTK
- ABB
- SIEMENS
- WEG
- OTHER

## Workflow

`source -> *.source.json raw transcription -> normalization -> source comparison -> confidence assignment -> import-ready *.json -> import Preview -> similarity check -> selected import`

Do not bypass Preview or similarity checking when importing catalogue records into a device.
