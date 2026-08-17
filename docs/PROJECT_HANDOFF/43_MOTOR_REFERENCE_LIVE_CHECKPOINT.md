# Motor Reference Live Checkpoint

Updated: 2026-08-17 13:20+06
Branch: `cmp-protocol-v1` only. Do not use `main` as source.

Rolling continuation point for the motor winding reference expansion. Update this file after every significant source/architecture block so a new chat can resume immediately.

## Non-negotiable rules
- Static/reference motor data stays separate from production motors.
- No automatic physical START, SSR control, auto-resume, or automatic wire write-off.
- `RUN_COMPLETED` alone never deducts wire.
- Wire write-off remains manual and tied to exact `spool_id`, `source_session_id`, `source_run_id`.
- Ambiguous notation remains source-native/review-required; never synthesize `coil_program` from uncertain N/pitch/parallelism/conductor data.
- `merge_only=true` must enrich exactly one base record by `series + model + variant_key`; it must not add duplicate cards.

## Current generated checkpoint
- `firmware/esp32/web/reference/motor-reference.json`: **1339 records**
- `reference_only=true`
- Count is merge-aware.
- 1253 -> 1271: `92c051d`, +18 AOK2 5–7 base cards.
- 1271 -> 1305: `415866a`, +34 senior plain-4A 280/315/355 base cards.
- 1305 -> 1339: `dd3f910`, +34 senior 4AM 280/315/355 base cards.
- `421e55c` is merge-only 4A280 electrical enrichment and does not increase count.

## Architecture already in place
- `tools/build_motor_reference.py`: aliases, spaced `model / alias`, `winding_sets_source`, strict `merge_only`.
- `tools/check_motor_reference.py`: merge-aware, validates supplement provenance via `source_files`.
- Desktop/mobile reference search includes aliases.

## Major coverage state
- AIR/AIS: serial/repair coverage, standalone AIS and aliases.
- 4A: standard through 250; technical multispeed 100–250; senior **4A280/315/355 base now present** with 34 models across 2/4/6/8/10/12P. `4A_280_ELECTRICAL_SUPPLEMENT.source.json` safely enriches six clean 280 rows (S/M 4P, 8P, 10P) with rpm/voltage/current. Detailed Pe1/m1/a1/w1/conductor remains pending row-safe parsing of table 8.21.
- 4AM: previously almost empty except multispeed observations; **senior 4AM280/315/355 base now present** with 34 model/rating identities. Winding is explicitly pending; never copy 4A winding values into 4AM without direct corroboration.
- 4AK: phase-rotor 160–250. Independent technical tables indicate ordinary 4AK line ends at 250; do NOT invent 4AK280+ merely because table 8.21 interleaves labels.
- 4ANK: phase-rotor 160–355 already contains rich separate stator/rotor data, including rectangular conductors. 4ANK280 source already has rotor pitch/conductor/mass/resistance, so avoid duplicating section 8.4 unless adding genuinely missing fields safely.
- 5A: low/mid/senior incl. 6P/8P/12P supplements.
- 6A: IN_PROGRESS; first source-native `6А90В4` has `uп=70`, `w1=420`, `a1=1`, `I=2.0 A`, explicit model-text conflict. No inferred remaining rows.
- A2: IN_PROGRESS index/reference + selective merge-only enrichment.
- AO2/AOL2/AOP2/AOS2/AOT2/AOK2: static plus multispeed frames 1–9, P=const/M=const kept distinct.
- AOK2: 18 base cards for frames 5–7; frame-4 electrical merge enrichment; frame 5 rotor conductor grade ПЭТВП, frames 6–7 ПСД, rotor Y.
- AK2: 12 base cards for 81/82/91/92 × 4P/6P/8P; rotor Y and bare-copper-bar/glass-tape construction merged.
- 4ANK/VAO provide substantial actual rectangular-conductor coverage; dedicated `RECTANGULAR_LV` source remains PLANNED because Vitkovoe section 18 is image-based in available search results.
- CRANE/LIFT/SINGLE_PHASE/GENERATORS/IMPORT/REPAIR/HV/DC/legacy A-AD-AL-AOL retained; avoid duplicate re-transcription.

## Recent commits
- `30da70a` preserve `winding_sets_source`
- `8f4b4b1` strict merge_only
- `6f81a14` merge-aware checker
- `5dcb83c` first 6A winding reference
- `e9c264c` AK2 8–9 base
- `92c051d` AOK2 5–7 base
- `32f66c8` AOK2 frame-4 electrical enrichment
- `639c9dc` AK2 rotor-construction enrichment
- `de2de05` AOK2 rotor-construction enrichment
- `415866a` senior 4A280–355 base (+34)
- `421e55c` 4A280 electrical merge enrichment
- `dd3f910` senior 4AM280–355 base (+34)
- `52002a3` created this live checkpoint

## Confirmed workflow checkpoints
- `5dcb83c`, `e9c264c`, `92c051d`, `32f66c8`, `639c9dc`, `de2de05`, `415866a`, `dd3f910`: `Motor reference index` success and `CMP Protocol Tests` success.
- Do not generalize these results to all project CI.

## Exact next continuation point
1. **Senior 4A table 8.21**: continue row-safe merge-only enrichment. Current safe subset already covers 4A280 S/M for 4P, 8P, 10P electrical data. Seek independent copies for Pe1/m1/a1/w1/conductor before writing winding columns.
2. **4AM winding sources**: senior 4AM base exists, but winding is pending. Search direct manufacturer/repair winding references; never transfer 4A winding automatically.
3. **AOK2/AK2 detailed rotor winding**: recover row-safe Z2/y2/Pe2/m2/a2/w2/conductor dimensions/mass/resistance from section 8.2.
4. **6A 80/90**: keep searching textual/mirrored form of image-based winding table; no OCR guessing.
5. Continue AO2/A2 and VAO merge-only enrichment; retry Vitkovoe rectangular-LV section 18 if a textual/mirrored table becomes available.

## Handoff maintenance rule
After every significant source package, generator/checker change, or new confirmed generated count: fetch this file from `cmp-protocol-v1`, use its current blob SHA, update count/commits/workflow status, and record the exact next continuation target.
