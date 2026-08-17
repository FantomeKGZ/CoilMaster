# CoilMaster source motor catalogue

This directory contains curated source packages for the CoilMaster motor and winding database.

It now serves two deliberately separated purposes:

1. a **read-only winding reference** containing source-native and not-yet-verified motor data;
2. a source for **working motor database imports** only after a record is normalized and sufficiently verified.

The project goal is maximum practical reference coverage without weakening the trust boundary of the working database. See `COVERAGE.md` for the complete population roadmap.

The read-only reference is allowed to contain uncertain, conflicting, compound or incomplete winding notation as long as provenance and warnings are preserved. The working motor database must not receive those records until the required CoilMaster semantics are proven.

## Layout

- one directory per motor series or manufacturer family;
- one source/staging JSON package per frame-size/model group while technical semantics are still being verified;
- one import-ready JSON package per frame-size/model group only after all required CoilMaster fields are supported by the source or a documented calculation;
- `catalog.json` is the catalogue index and population-status registry;
- `COVERAGE.md` defines the maximum-coverage target and population order;
- import packages must follow `docs/MOTOR_IMPORT_FORMAT.md`;
- package size must remain within the importer limit of 1–50 records.

File naming contract:

- `*.source.json` — raw/source-native transcription; **reference-only / not import-ready** and must never be submitted to the device motor importer;
- `*.json` without `.source` — import-ready package and must strictly follow `docs/MOTOR_IMPORT_FORMAT.md`.

Recommended naming examples:

- `AIR/AIR_71.source.json` — source/reference transcription;
- `AIR/AIR_71.json` — import-ready normalized package;
- `AIR/AIR_80.json`
- `4A/4A_100.json`
- `ABB/M3AA_160.json`

## Static winding reference

The device UI has a separate read-only reference page:

- desktop: `/desktop/winding-reference.html`;
- mobile: `/mobile/winding-reference.html`.

The page reads `/reference/motor-reference.json`. That index is generated from all `*.source.json` files by:

`python tools/build_motor_reference.py`

The reference navigation/filter model is:

`manufacturer or series -> speed group -> slot count -> model/variant`

Reference records may display source-native `N`, wire construction, pitch, stator geometry, connection, current/voltage text, provenance and review warnings. They are informational only and provide no physical START path, no SSR control and no automatic creation of a working motor card.

Promotion into the working database is a separate operation:

`reference/source record -> technical review -> deterministic normalization -> import-ready *.json -> import Preview -> similarity check -> selected import`

The original `*.source.json` record remains unchanged after promotion so provenance is never lost.

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
13. A record may appear in the static reference even when rule 12 is not yet satisfied; it must remain visibly reference-only and must not be used as a machine program.

## Coverage scope

The catalogue is intended to grow beyond a few popular series. Tracked coverage includes:

- AIR / AIS;
- 4A / 4AH, 4AM / 4AI / 4AIM;
- 5A and 6A families;
- A / AD / AL / AOL legacy families;
- A2 / AO2 and related AOS2 / AOT2 / AOL2 / AOLS2 variants;
- multispeed AO / AO2 motors;
- crane-motor families;
- repair-observation records from mixed motor series;
- ABB, Siemens, WEG, SEW-Eurodrive, NORD and other imported manufacturers;
- unidentified/rare records retained separately until identification is reliable.

`catalog.json` is the machine-readable population registry; `COVERAGE.md` is the human-readable completeness roadmap.

## Workflow

Reference path:

`source -> *.source.json -> static reference index -> read-only winding reference`

Working database path:

`source -> *.source.json -> normalization -> source comparison -> confidence assignment -> import-ready *.json -> import Preview -> similarity check -> selected import`

Do not bypass Preview or similarity checking when importing catalogue records into a device.
