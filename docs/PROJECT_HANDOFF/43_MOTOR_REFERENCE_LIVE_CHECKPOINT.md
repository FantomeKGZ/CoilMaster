# Motor Reference Live Checkpoint

Updated: 2026-08-17 13:15+06
Branch: `cmp-protocol-v1` only. Do not use `main` as source.

This file is the rolling continuation point for the motor winding reference expansion. Update it after every significant source/architecture block so a new chat can resume immediately without reconstructing prior work.

## Non-negotiable project rules
- Static/reference motor data stays separate from the production motor database.
- Reference data never causes automatic physical START, SSR control, auto-resume, or automatic wire write-off.
- `RUN_COMPLETED` alone never deducts wire.
- Wire write-off remains manual and tied to exact `spool_id`, `source_session_id`, `source_run_id`.
- Ambiguous source notation is preserved source-native and remains review-required; do not synthesize `coil_program` from uncertain `N`, pitch, parallelism, or conductor notation.
- `merge_only=true` enriches exactly one existing base record by `series + model + variant_key`; it must not create duplicate cards.

## Current generated checkpoint
- `firmware/esp32/web/reference/motor-reference.json`: **1305 records**
- `reference_only=true`
- Count is merge-aware; enrichment supplements do not inflate `record_count`.
- `92c051d` added 18 AOK2 5–7 frame base cards, moving 1253 -> 1271.
- `415866a` added 34 senior plain-4A 280/315/355 base cards, moving 1271 -> 1305.
- AOK2/AK2 rotor supplements are `merge_only`; they do not increase the count.

## Current architecture additions
- `tools/build_motor_reference.py` supports `aliases_in_source`, spaced `model / alias` source identities, `winding_sets_source`, and strict `merge_only` enrichment.
- `tools/check_motor_reference.py` is merge-aware and validates supplement provenance via `source_files`.
- Desktop and mobile winding-reference search include aliases.

## Major reference areas already added/expanded
- AIR/AIS: AIR serial/repair coverage plus standalone AIS records and aliases.
- 4A: standard frames through 250; technical multispeed 4A100–250; **senior 4A280/315/355 base layer now added from winding table 8.21** with 34 plain-4A models across 2/4/6/8/10/12 poles. Senior detailed winding columns remain pending row-safe merge-only enrichment.
- 4AK: phase-rotor 160–250 source-native stator/rotor data. Senior 4AK280/315/355 from section 8.4 is the current high-priority coverage gap; verify exact existing paths/models before adding bases.
- 4ANK: phase-rotor 160–355; strong rectangular-conductor coverage; stator/rotor kept separate. Section 8.4 can now enrich senior rotors with exact U2/I2 and, where row-safe, Z2/y2/Pe2/m2/a2/wf/conductor/mass/resistance.
- 4AM: known multispeed/repair observations only; no inferred transfer from 4A/AIR.
- 5A: low/mid/senior reference, including senior 6P, 8P and 12P supplements.
- 6A: **IN_PROGRESS**. First source-native `6А90В4` record contains `uп=70`, `w1=420`, `a1=1`, `I=2.0 A`, with explicit source model-text conflict flag. Do not infer remaining 6A rows.
- A2: **IN_PROGRESS** index/reference layer for safely readable senior models; selective merge-only geometry enrichment.
- AO2/AOL2/AOP2/AOS2/AOT2/AOK2: static reference plus multispeed AO2/AOL2 model layer for frames 1–9, preserving separate P=const / M=const variants.
- AOK2 phase-rotor:
  - existing 4th-frame AOK2 stator cards remain in `AO2_41_42.source.json`;
  - `AOK2_5_7_PHASE_ROTOR_INDEX.source.json` adds 18 AOK2 5–7 identities;
  - frame-4 electrical supplement adds rated speed, stator current, rotor current/voltage and rotor Y;
  - 5–7 construction supplement adds rotor Y plus `ПЭТВП` for frame 5 and `ПСД` for frames 6–7;
  - detailed Pe2/m2/a2/w2/pitch/dimensions remain pending row-safe section 8.2 mapping.
- AO_MULTI: Vitkovoe mixed multispeed plus technical 4A multispeed and AO2/AOL2 technical layers.
- AK2: 12 base cards for AK2-81/82/91/92 × 4P/6P/8P; all merge-enriched with Y rotor connection and bare copper bar/glass-tape construction. Exact bar dimensions remain pending row-safe mapping.
- CRANE, LIFT, SINGLE_PHASE, GENERATORS, IMPORT, REPAIR_RECORDS, HV, DC index, legacy A/AD/AL/AOL: existing source layers retained; avoid duplicate re-transcription.
- VAO: IN_PROGRESS. Base index for 5–9 plus 315/355/450; multispeed VAO 6–9; rectangular stator enrichment for VAO450 and VAO355 10P.

## Recent important commits
- `30da70a` generator: preserve `winding_sets_source`
- `8f4b4b1` strict `merge_only`
- `6f81a14` merge-aware checker
- `83c493f` 4A132 technical multispeed
- `6c90a07` senior 4A technical multispeed
- `5465b30` 4A 6/4/2 technical multispeed
- `e036e51` VAO multispeed 6–9 base
- `937beab` VAO 9 multispeed enrichment
- `80c2218` AO2 multispeed frame-3 geometry enrichment
- `913c999` AO2 frame-4 4/2 enrichment
- `3058d80` VAO 7–8 voltage/connection enrichment
- `6185aca` A2-82-4 geometry enrichment
- `5dcb83c` first 6A winding-reference record
- `3c18588` catalog: 6A -> IN_PROGRESS
- `e9c264c` AK2 8–9 phase-rotor base index
- `b2ce93c` catalog: register AK2 PHASE_ROTOR
- `92c051d` AOK2 5–7 phase-rotor reference index (+18 base cards)
- `32f66c8` AOK2 frame-4 phase-rotor electrical merge enrichment
- `639c9dc` AK2 8–9 rotor construction merge enrichment
- `de2de05` AOK2 5–7 rotor construction merge enrichment
- `415866a` senior 4A280/315/355 base index (+34 cards)
- `52002a3` created this live checkpoint

## Confirmed workflow checkpoints
- `5dcb83c`: Motor reference index success; CMP Protocol Tests success.
- `e9c264c`: Motor reference index success; CMP Protocol Tests success.
- `6185aca`: Motor reference index success; CMP Protocol Tests success.
- `92c051d`: Motor reference index success; CMP Protocol Tests success.
- `32f66c8`: Motor reference index success; CMP Protocol Tests success.
- `639c9dc`: Motor reference index success; CMP Protocol Tests success.
- `de2de05`: Motor reference index success; CMP Protocol Tests success.
- `415866a`: Motor reference index success; CMP Protocol Tests success.
- Do not generalize these to all project CI unless separately confirmed.

## Exact next continuation point
1. **Senior 4AK/4ANK section 8.4**: inspect `data/motor_catalog/4ANK` and exact senior model presence. Existing `4AK` directory currently ends at frame 250, so verify `4AK280S4`, `4AK315S4`, `4AK355S4` etc. If absent, add safe senior 4AK base cards from section 8.4 after exact-path 404 checks. Enrich existing 4ANK senior cards via `merge_only` with row-safe U2/I2 and Z2/y2/Pe2/m2/a2/wf/conductor/mass/resistance.
2. **Senior plain 4A table 8.21 enrichment**: base 34 cards exist; add detailed stator data only where model-to-column mapping is unambiguous.
3. **AOK2/AK2 detailed rotor winding**: recover exact row mapping for Z2/y2/Pe2/m2/a2/w2/conductor dimensions/mass/resistance from section 8.2.
4. **6A 80/90**: continue searching textual/mirrored form of image-based winding table; no OCR guessing.
5. Continue AO2/A2 and VAO merge-only enrichment, then dedicated rectangular-LV discovery.

## Handoff maintenance rule
After every significant source package, generator/checker change, or new confirmed generated count:
1. fetch this file from `cmp-protocol-v1` and use its current blob SHA;
2. update the relevant sections above;
3. record the new commits and exact generated `record_count`;
4. record the next precise continuation target.
