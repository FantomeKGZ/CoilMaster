# CoilMaster winding reference coverage

Goal: build the broadest practical read-only winding reference while keeping the working CoilMaster motor database limited to technically reviewed and normalized records.

## Coverage policy

The static reference may contain incomplete, conflicting, compound, repair-observed, legacy and manufacturer-specific winding notation when the source is preserved and the record is clearly reference-only.

The working motor database remains stricter: a record is promoted only after its winding semantics are understood well enough to create a deterministic CoilMaster program and the normal import Preview/similarity workflow is passed.

Completeness therefore means **source coverage**, not pretending every record is verified.

## Target source families

### General-purpose CIS / Soviet

- AIR / AIS
- 4A / 4AH
- 4AM / 4AI / 4AIM
- 5A / 5AI / 5AM / 5AMX
- 6A family
- A / AD / AL / AOL
- A2 / AO2 and related AOS2, AOT2, AOL2, AOLS2 variants

### Multispeed

- AO / AO2 two-speed and other multispeed variants
- preserve every speed/pole combination, voltage and connection separately
- do not force multispeed connection schemes into the current single-speed working schema

### Crane motors

- MT
- MTK
- MTF
- MTH / MTN source-labelled variants
- other crane-family records retained under the exact source model designation

### Field / repair observations

- motors received for repair with measured winding data
- preserve exact geometry, speed, voltage, connection, conductor construction and source notes
- never merge a repair observation with a catalogue row merely because the model name is similar

### Imported manufacturers

Priority manufacturers:

- ABB
- Siemens
- WEG
- SEW-Eurodrive
- NORD
- other imported motors for which winding data can be sourced or physically verified

Imported motors without a stable manufacturer-family mapping remain in `OTHER_IMPORT` until identified.

## Reference navigation

Primary browse path:

`series/manufacturer -> speed group -> stator slot count -> model/variant`

Secondary filters should eventually include:

- power
- voltage
- connection
- pole count / speed combination
- stator bore
- stator core length
- winding pitch
- wire construction
- source confidence / review state

## Status meanings

- `REFERENCE`: source-native data suitable for lookup but not automatically a machine program.
- `REVIEW_REQUIRED`: source anomaly, conflict, compound notation or unresolved construction semantics.
- `VERIFIED` is reserved for the working/verified layer and must not be inferred merely because a value appears in a reference table.

## Current population

At the time this roadmap was created:

- AIR raw/reference coverage: frame sizes 71-160.
- 4A raw/reference coverage: frame sizes 50-250.
- the generated device reference index contains the accumulated source records and is rebuilt automatically from `*.source.json`.

## Population order

1. Finish source-covered 4A range.
2. Add the complete A2/AO2/AOL2/AOS2/AOT2 table family.
3. Add 5A family.
4. Add 6A family.
5. Add A/AD/AL/AOL legacy family.
6. Add multispeed AO/AO2 tables.
7. Add crane-motor tables.
8. Import the broad mixed repair-observation table as reference-only records.
9. Expand imported-manufacturer coverage from technical documents and verified repair observations.
10. Continuously deduplicate copied tables by source-family/independence group rather than by URL alone.

## Quality rule

A large reference with explicit uncertainty is preferable to a smaller reference that silently discards useful source data. A large **working database** with uncertain winding programs is not acceptable. The separation between these two layers is permanent.
