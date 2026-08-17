# Motor Reference Live Checkpoint

Updated: 2026-08-17 13:25+06
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
- Later 4A/4AM/VAO supplements are merge-only and do not increase count.

## Architecture already in place
- `tools/build_motor_reference.py`: aliases, spaced `model / alias`, `winding_sets_source`, strict `merge_only`.
- `tools/check_motor_reference.py`: merge-aware, validates supplement provenance via `source_files`.
- Desktop/mobile reference search includes aliases.

## Major coverage state
- AIR/AIS: serial/repair coverage, standalone AIS and aliases.
- 4A: standard through 250; technical multispeed 100–250; senior 4A280/315/355 base with 34 models across 2/4/6/8/10/12P. `4A_280_ELECTRICAL_SUPPLEMENT.source.json` enriches six clean 280 rows (S/M 4P, 8P, 10P) with rpm/voltage/current. Detailed Pe1/m1/a1/w1/conductor remains pending row-safe parsing of table 8.21.
- 4AM: senior 4AM280/315/355 base with 34 model/rating identities. `4AM_SENIOR_ELECTRICAL_SUPPLEMENT.source.json` enriches clean 4P cards (280S/M, 315S/M) with voltage/current/rpm where explicit. Winding remains pending; never copy 4A winding automatically.
- 4AK: phase-rotor 160–250. Independent technical tables indicate ordinary 4AK line ends at 250; do NOT invent 4AK280+.
- 4ANK: phase-rotor 160–355 already contains rich separate stator/rotor data, including rectangular conductors. 4ANK280 already has rotor pitch/conductor/mass/resistance; avoid duplicate retranscription.
- 5A: low/mid/senior incl. 6P/8P/12P supplements.
- 6A: IN_PROGRESS; first source-native `6А90В4` has `uп=70`, `w1=420`, `a1=1`, `I=2.0 A`, explicit model-text conflict. No inferred remaining rows.
- A2: IN_PROGRESS index/reference + selective merge-only enrichment.
- AO2/AOL2/AOP2/AOS2/AOT2/AOK2: static plus multispeed frames 1–9, P=const/M=const kept distinct.
- AOK2: 18 base cards for frames 5–7; frame-4 electrical enrichment; frame 5 rotor conductor ПЭТВП, frames 6–7 ПСД, rotor Y.
- AK2: 12 base cards 81/82/91/92 × 4P/6P/8P; rotor Y and bare-copper-bar/glass-tape construction merged.
- VAO high-frame:
  - base `VAO_315_450_INDEX.source.json` retained;
  - VAO450 already has full rectangular stator enrichment;
  - VAO355M/L-10 existing supplement retained;
  - `VAO_355_2P_STATOR_SUPPLEMENT.source.json` adds a fully row-safe rectangular stator for ВАО355M-2: 590/320, L335, gap2.0, Z48, pitch1-15, Pe1=16, m1=2, a1=2, wf32, conductor 2.63×6.9, mass106.5, r1=0.0135;
  - `VAO_355_GEOMETRY_STATOR_SUPPLEMENT.source.json` merge-enriches remaining VAO355 L2/M4/L4/M6/L6/M8/L8 with row-safe current, geometry, gap, Z1, pitch, Pe1 and m1; a1/wf/conductor intentionally omitted where HTML columns interleave;
  - `VAO_315_GEOMETRY_STATOR_SUPPLEMENT.source.json` merge-enriches existing VAO315 S/M 2P/4P with row-safe current, geometry, gap, Z1/pitch and Pe1 where clear. Do not add OCR-ambiguous VAO315 S/M 6/8/10 identities until independently resolved.
- 4ANK/VAO provide substantial actual rectangular-conductor coverage; dedicated `RECTANGULAR_LV` remains PLANNED because Vitkovoe section 18 is image-based in current retrieval.
- CRANE/LIFT/SINGLE_PHASE/GENERATORS/IMPORT/REPAIR/HV/DC/legacy A-AD-AL-AOL retained; avoid duplicates.

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
- `2ac707d` senior 4AM electrical merge enrichment
- `2df1c43` VAO355M-2 full rectangular stator enrichment
- `b6e4f63` VAO355 partial stator/geometry enrichment
- `7a687bd` VAO315 partial stator/geometry enrichment
- `52002a3` created this live checkpoint

## Confirmed workflow checkpoints
- `5dcb83c`, `e9c264c`, `92c051d`, `32f66c8`, `639c9dc`, `de2de05`, `415866a`, `dd3f910`, `2ac707d`, `b6e4f63`: Motor reference index success and CMP Protocol Tests success.
- `7a687bd`: workflow was still in progress at this exact checkpoint update; do not call it green until rechecked.
- Do not generalize these results to all project CI.

## Exact next continuation point
1. Recheck `7a687bd` workflows. Keep record_count 1339 because all VAO changes are merge-only.
2. Continue VAO315 only for the four existing unambiguous base cards; do NOT create the OCR-ambiguous 6/8/10 identities until independently resolved.
3. Senior 4A table 8.21: seek independent copies for Pe1/m1/a1/w1/conductor before writing winding columns.
4. 4AM direct winding sources only; no automatic transfer from 4A.
5. AOK2/AK2 detailed rotor winding: recover row-safe Z2/y2/Pe2/m2/a2/w2/conductor dimensions/mass/resistance from section 8.2.
6. 6A80/90 textual/mirrored winding table; no OCR guessing.
7. Continue AO2/A2 and VAO merge-only enrichment; retry rectangular-LV section 18 if textual mirror appears.

## Handoff maintenance rule
After every significant source package, generator/checker change, or new confirmed generated count: fetch this file from `cmp-protocol-v1`, use its current blob SHA, update count/commits/workflow status, and record the exact next continuation target.
