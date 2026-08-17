# Motor Reference Live Checkpoint

Updated: 2026-08-17 13:34+06
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
- `firmware/esp32/web/reference/motor-reference.json`: **1368 records**
- `reference_only=true`
- Count is merge-aware.
- 1253 -> 1271: `92c051d`, +18 AOK2 5–7 base cards.
- 1271 -> 1305: `415866a`, +34 senior 4A280/315/355 base cards.
- 1305 -> 1339: `dd3f910`, +34 senior 4AM280/315/355 base cards.
- 1339 -> 1358: standard single-speed AO2 frames 5–9 initial expansion, +19 cards.
- 1358 -> 1366: `f2b44b6`, +8 additional AO2 frame-8/9 cards.
- 1366 -> 1368: `a987b13`, +2 AO2 frame-7 4P cards.
- Later 4A/4AM/VAO supplements are merge-only and do not increase count.

## Architecture already in place
- `tools/build_motor_reference.py`: aliases, spaced `model / alias`, `winding_sets_source`, strict `merge_only`.
- `tools/check_motor_reference.py`: merge-aware, validates supplement provenance via `source_files`.
- Desktop/mobile reference search includes aliases.

## Major coverage state
- AIR/AIS: serial/repair coverage, standalone AIS and aliases.
- 4A: standard through 250; technical multispeed 100–250; senior 4A280/315/355 base with 34 models across 2/4/6/8/10/12P. `4A_280_ELECTRICAL_SUPPLEMENT.source.json` enriches six clean 280 rows with rpm/voltage/current. Detailed Pe1/m1/a1/w1/conductor remains pending row-safe parsing of table 8.21.
- 4AM: senior 4AM280/315/355 base with 34 model/rating identities. `4AM_SENIOR_ELECTRICAL_SUPPLEMENT.source.json` enriches clean 4P cards (280S/M, 315S/M). Winding remains pending; never copy 4A winding automatically.
- 4AK: phase-rotor 160–250. Ordinary 4AK line ends at 250; do NOT invent 4AK280+.
- 4ANK: phase-rotor 160–355 with rich separate stator/rotor rectangular-conductor data.
- 5A: low/mid/senior incl. 6P/8P/12P supplements.
- 6A: IN_PROGRESS; first source-native `6А90В4` has `uп=70`, `w1=420`, `a1=1`, `I=2.0 A`, explicit model-text conflict. No inferred remaining rows.
- A2: IN_PROGRESS index/reference + selective merge-only enrichment.
- AO2 static standard coverage now extends beyond prior frame 4 into frames 5–9 from Likhachev 2004. New source files:
  - `AO2_5_STANDARD_TECHNICAL_01.source.json` (`cf9526c`): AO2-51-4, AO2-51-6.
  - `AO2_5_STANDARD_TECHNICAL_02.source.json` (`3891489`): AO2-52-4/6/8.
  - `AO2_6_STANDARD_TECHNICAL_01.source.json` (`ada97bf`): AO2-61-6, AO2-62-4/6.
  - `AO2_7_STANDARD_TECHNICAL_01.source.json` (`d5a0197`): AO2-71/72-6 and AO2-71/72-8.
  - `AO2_8_9_STANDARD_TECHNICAL_01.source.json` (`5f6b8fd`): AO2-81/82-6, AO2-81-8, AO2-91/92-4, AO2-92-6, AO2-91-8.
  - `AO2_8_9_STANDARD_TECHNICAL_02.source.json` (`f2b44b6`): AO2-81/82-2, AO2-81-4, AO2-91/92-2, AO2-92-8, AO2-91/92-10. AO2-92-2 current `312/108` is preserved literally and flagged suspect.
  - `AO2_7_STANDARD_TECHNICAL_02.source.json` (`a987b13`): AO2-71-4 (22kW/1455/71.5/41.2), AO2-72-4 (30kW/1455; current intentionally omitted due interleaving).
  - Remaining AO2 standard gaps include clean resolution of AO2-82-8, AO2-91-6, AO2-71/72-2, and any other omitted rows where the HTML transcription interleaves modifications.
- AO2/AOL2 multispeed frames 1–9 already tracked separately in AO_MULTI; P=const/M=const remain distinct.
- AOK2: 18 base cards frames 5–7; frame-4 electrical enrichment; frame5 rotor conductor ПЭТВП, frames6–7 ПСД, rotor Y.
- AK2: 12 base cards 81/82/91/92 ×4/6/8P; rotor Y and bare-copper-bar/glass-tape construction merged.
- VAO high-frame: VAO450 rectangular stator, VAO355 full/partial stator enrichments, VAO315 S/M 2P/4P partial enrichment. OCR-ambiguous VAO315 6/8/10 identities remain excluded.
- 4ANK/VAO provide substantial actual rectangular-conductor coverage; dedicated `RECTANGULAR_LV` remains PLANNED because Vitkovoe section 18 is image-based in current retrieval.
- CRANE/LIFT/SINGLE_PHASE/GENERATORS/IMPORT/REPAIR/HV/DC/legacy A-AD-AL-AOL retained; avoid duplicates.

## Recent important commits
- `30da70a` preserve `winding_sets_source`; `8f4b4b1` strict merge_only; `6f81a14` merge-aware checker.
- `415866a` senior 4A280–355 base; `421e55c` 4A280 electrical enrichment.
- `dd3f910` senior 4AM280–355 base; `2ac707d` senior 4AM electrical enrichment.
- `2df1c43`, `b6e4f63`, `7a687bd` VAO high-frame enrichments.
- `cf9526c`, `3891489`, `ada97bf`, `d5a0197`, `5f6b8fd`, `f2b44b6`, `a987b13` standard AO2 frames 5–9 expansion.
- `52002a3` created this live checkpoint.

## Confirmed workflow checkpoints
- `7a687bd`, `5f6b8fd`, `f2b44b6`, `a987b13`: Motor reference index success and CMP Protocol Tests success.
- Earlier recorded source/generator commits remain as previously confirmed in history.
- Do not generalize these results to all project CI.

## Exact next continuation point
1. Continue standard AO2 single-speed gaps: resolve AO2-82-8 and AO2-91-6 only with row-safe mapping; add AO2-71/72-2 if not already represented elsewhere after duplicate check.
2. Add merge-only detailed winding fields to new AO2 frame5–9 cards only where Z/pitch/Pe1/m1/a1/w1/conductor maps cleanly; preserve suspicious values literally.
3. Senior 4A table 8.21: seek independent copies for Pe1/m1/a1/w1/conductor before writing winding columns.
4. 4AM direct winding sources only; no automatic transfer from 4A.
5. AOK2/AK2 detailed rotor winding: recover row-safe Z2/y2/Pe2/m2/a2/w2/conductor dimensions/mass/resistance from section 8.2.
6. 6A80/90 textual/mirrored winding table; no OCR guessing.
7. Continue VAO enrichment and retry rectangular-LV section 18 if textual mirror appears.

## Handoff maintenance rule
After every significant source package, generator/checker change, or new confirmed generated count: fetch this file from `cmp-protocol-v1`, use its current blob SHA, update count/commits/workflow status, and record the exact next continuation target.
