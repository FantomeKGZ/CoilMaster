# Motor Reference Live Checkpoint

Updated: 2026-08-17
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
- `firmware/esp32/web/reference/motor-reference.json`: **1253 records**
- `reference_only=true`
- Count is merge-aware; enrichment supplements do not inflate `record_count`.

## Current architecture additions
- `tools/build_motor_reference.py` supports `aliases_in_source`, spaced `model / alias` source identities, `winding_sets_source`, and strict `merge_only` enrichment.
- `tools/check_motor_reference.py` is merge-aware and validates supplement provenance via `source_files`.
- Desktop and mobile winding-reference search include aliases.

## Major reference areas already added/expanded
- AIR/AIS: AIR serial/repair coverage plus standalone AIS records and aliases.
- 4A: standard frames through 250; technical multispeed 4A100–250 including 4/2, 6/4/2, 8/4/2, 8/6/4, 12/8/6/4 variants.
- 4AK: phase-rotor 160–250 source-native stator/rotor data.
- 4ANK: phase-rotor 160–355; strong rectangular-conductor coverage; stator/rotor kept separate.
- 4AM: known multispeed/repair observations only; no inferred transfer from 4A/AIR.
- 5A: low/mid/senior reference, including senior 6P, 8P and 12P supplements.
- 6A: **IN_PROGRESS**. First source-native `6А90В4` record added from corroborating technical material; contains `uп=70`, `w1=420`, `a1=1`, `I=2.0 A`, with explicit source model-text conflict flag. Do not infer remaining 6A table rows.
- A2: **IN_PROGRESS** index/reference layer for safely readable senior models; selective merge-only geometry enrichment.
- AO2/AOL2/AOP2/AOS2/AOT2/AOK2: static reference through currently captured frames; multispeed AO2/AOL2 model layer for frames 1–9 added with separate P=const / M=const variants where source distinguishes them.
- AO_MULTI: Vitkovoe mixed multispeed plus technical 4A multispeed and AO2/AOL2 technical index layers.
- AK2: **IN_PROGRESS PHASE_ROTOR**. 12 base cards for AK2-81/82/91/92 × 4P/6P/8P added with stator current and rotor current/voltage source fields.
- CRANE, LIFT, SINGLE_PHASE, GENERATORS, IMPORT, REPAIR_RECORDS, HV, DC index, legacy A/AD/AL/AOL: existing source layers retained; avoid duplicate re-transcription.
- VAO: **IN_PROGRESS**. Base index for 5–9 frames plus 315/355/450; multispeed VAO 6–9 added; rectangular stator enrichment for VAO450 and VAO355 10P; selective geometry/winding enrichment via merge-only.

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
- `5816c1b` previous 6A/AK2/VAO handoff checkpoint

## Confirmed workflow checkpoints
- `5dcb83c`: `Motor reference index` success; `CMP Protocol Tests` success.
- `e9c264c`: `Motor reference index` success; `CMP Protocol Tests` success.
- `6185aca`: `Motor reference index` success; `CMP Protocol Tests` success.
- Do not generalize these to all project CI unless separately confirmed.

## Exact next continuation point
1. **AOK2 rotor enrichment**: existing AOK2-41/42 stator cards are already in `AO2_41_42.source.json`; add rotor data only via `merge_only`, never duplicate them as new motors.
2. **AK2 rotor/winding enrichment**: use technical table 8.2 where exact row-to-column mapping is readable.
3. **6A 80/90**: keep searching for a textual/mirrored form of the image-based winding table. Add only confirmed rows; no OCR guessing.
4. **4A280/315/355**: separate winding-data section is known to exist, but current HTML extraction does not expose reliable columns yet. Add only when `N / conductor / pitch` map cleanly.
5. Continue AO2/A2 merge-only enrichment, VAO winding enrichment, then dedicated rectangular-LV source discovery.

## Handoff maintenance rule
After every significant source package, generator/checker change, or new confirmed generated count:
1. fetch this file from `cmp-protocol-v1` and use its current blob SHA;
2. update the relevant sections above;
3. record the new commits and exact generated `record_count`;
4. record the next precise continuation target.
