# CoilMaster motor-reference expansion — 2026-08-17 checkpoint 42

## Confirmed static reference checkpoint

- Branch/source of truth: `cmp-protocol-v1`.
- `firmware/esp32/web/reference/motor-reference.json`: `record_count = 1253`.
- `reference_only = true`.
- The generated reference remains read-only and does not create `coil_program` or working motor records.

## New VAO multispeed layer

- `data/motor_catalog/VAO/VAO_MULTISPEED_6_9_INDEX.source.json`
  - 16 model/reference cards for VAO 6th–9th frames.
  - Includes 4/12, 4/8, 4/6, 4/6/8 and 4/6/8/12 pole combinations where explicitly present in Likhachev section 8.7.4.
- `data/motor_catalog/VAO/VAO_MULTISPEED_9_STATOR_SUPPLEMENT.source.json`
  - `merge_only=true` enrichment for six 9th-frame multispeed VAO cards.
  - Preserves geometry, slots and only the speed/power/current/connection/pitch groups whose row mapping is unambiguous.
- `data/motor_catalog/VAO/VAO_7_8_CONNECTION_SUPPLEMENT.source.json`
  - `merge_only=true`.
  - Adds explicit 380/660 V and DELTA/Y source connection to ordinary VAO 7th/8th-frame cards without increasing record count.

## AO2 multispeed enrichment

- `data/motor_catalog/AO_MULTI/AO2_MULTISPEED_3_GEOMETRY_SUPPLEMENT.source.json`
  - `merge_only=true` geometry enrichment for all 16 existing 3rd-frame AOL2/AO2 multispeed variants.
  - P=const and M=const 6/4 variants remain distinct by `variant_key`.
- `data/motor_catalog/AO_MULTI/AO2_MULTISPEED_4_GEOMETRY_SUPPLEMENT.source.json`
  - `merge_only=true`.
  - Only the unambiguous AO2-41-4/2 and AO2-42-4/2 geometry/pitch rows are merged; interleaved 6/4, 8/4 and 6/4/2 rows remain pending.

## A2 enrichment

- `data/motor_catalog/A2/A2_82_4_GEOMETRY_SUPPLEMENT.source.json`
  - `merge_only=true`.
  - A2-82-4: 50 kW, 1460 rpm, 176/102 A, Da 393, Di 247, L 190, air gap 0.9 mm.
  - Turn/conductor columns remain pending rather than reconstructed from ambiguous HTML ordering.

## 6A is now IN_PROGRESS

- `data/motor_catalog/6A/6A_90V4_CORROBORATION_01.source.json`
  - First real 6A source-native winding record.
  - Explicit source values for 6A90V4: effective slot conductors `uп=70`, phase turns `w1=420`, parallel branches `a1=1`, nominal current 2.0 A.
  - Separate 6A 80–90 winding article corroborates family context: 380 V and star connection.
  - Source text contains an internal conflict: the given data names 6A90V4 while a later task sentence says 6A80V4. The record is therefore `REVIEW_REQUIRED_SOURCE_TEXT_MODEL_CONFLICT`.
  - No wire diameter, pitch, slot count or `coil_program` is inferred.
- `data/motor_catalog/catalog.json`: 6A changed `PLANNED -> IN_PROGRESS`.

## AK2 phase-rotor family opened

- `data/motor_catalog/AK2/AK2_8_9_INDEX.source.json`
  - 12 independent phase-rotor cards: 81/82/91/92 × 4P/6P/8P.
  - Captures clean technical ratings plus rotor current/voltage from phase-rotor technical tables.
  - Detailed winding conductor/turn data remains pending until section 8.2 can be mapped without ambiguity.
- `data/motor_catalog/catalog.json`: new `AK2` PHASE_ROTOR series, status `IN_PROGRESS`.
- Existing AOK2 stator cards stay in `AO2`; future AOK2 rotor detail should be added through strict `merge_only` enrichment rather than duplicated as new physical cards.

## Generator / integrity rules retained

- `merge_only=true` supplements must match exactly one base card by series/model/variant key; ambiguity fails generation.
- `winding_sets_source` is preserved in the static index.
- Source-native quantities such as `Sп`, `n/a`, effective conductors, phase turns and rectangular conductor dimensions are not silently converted into `coil_program`, `parallel_strands` or another semantic field.
- Reference cards never automatically enter production motor workflow.

## Confirmed workflow facts

- First 6A source commit `5dcb83c2e162b8b244e1f3f160c35eb097916837`: `Motor reference index = success`, `CMP Protocol Tests = success`.
- AK2 source commit `e9c264cd3214d6031c0d1a68a71495a56319d98e`: `Motor reference index = success`, `CMP Protocol Tests = success`.
- A2-82-4 merge-only enrichment commit `6185aca36df9e439da5c2a4383a7307be5a5afa0`: `Motor reference index = success`, `CMP Protocol Tests = success`.
- These statements are specific to the listed workflows/commits and do not assert that all project CI is green.

## Immediate continuation

1. AOK2 rotor enrichment from section 8.2, using existing AO2/AOK2 stator cards as merge-only bases.
2. Continue recovering 6A80/90 detailed image-table rows through text mirrors/corroborating technical sources; never infer missing wire/pitch.
3. Recover 4A280/315/355 winding data from the dedicated reference section; current 4A raw layer stops at frame 250.
4. Continue A2/AO2 1–9 winding enrichment where row-to-column mapping can be proven.
5. Continue rectangular-wire coverage through actual model-specific stator/rotor rows rather than general theory.
