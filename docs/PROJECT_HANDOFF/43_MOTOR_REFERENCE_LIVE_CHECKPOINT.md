# Motor Reference Live Checkpoint

Updated: 2026-08-17 14:12+06
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
- `firmware/esp32/web/reference/motor-reference.json`: **1390 records**
- `reference_only=true`
- Count is merge-aware.
- 1253 -> 1271: `92c051d`, +18 AOK2 5–7 base cards.
- 1271 -> 1305: `415866a`, +34 senior plain-4A 280/315/355 base cards.
- 1305 -> 1339: `dd3f910`, +34 senior 4AM 280/315/355 base cards.
- 1339 -> 1358: initial standard single-speed AO2 frames 5–9 expansion, +19 cards.
- 1358 -> 1366: `f2b44b6`, +8 AO2 frame-8/9 cards.
- 1366 -> 1368: `a987b13`, +2 AO2 frame-7 4P cards.
- 1368 -> 1372: `16bc71c`, +4 AO2 standard gap-fill cards.
- 1372 -> 1380: `3d74222`, +8 additional AO2 standard rating cards.
- 1380 -> 1382: `1f51cc7`, +2 final obvious AO2 standard rating gaps.
- 1382 -> 1386: `6c3c23d`, +4 senior 5AM 10-pole base cards.
- 1386 -> 1388: `99ea02b`, +2 missing senior 5AH315 8-pole winding cards.
- 1388 -> 1390: `9428ca5`, +2 missing 5A160M8 / 5A180M8 winding cards.
- `5bb442b`, `00e4ecf`, `513cbc5` are merge-only enrichments and do not increase count.
- A transient branch race created four duplicate AO2 cards via `66563b8` and `dbc4ec8`; those duplicate source files were removed by `621f79a` and `428ba8e` and canonical count returned to 1382 before later legitimate additions.
- Two later redundant AO2 geometry supplements were removed by `498cb6d` and `7b280f1`; authoritative AO2 gap enrichment remains `00e4ecf`.

## Architecture already in place
- `tools/build_motor_reference.py`: aliases, spaced `model / alias`, `winding_sets_source`, strict `merge_only`.
- `tools/check_motor_reference.py`: merge-aware, validates supplement provenance via `source_files`.
- Desktop/mobile reference search includes aliases.

## Major coverage state
- AIR/AIS: serial/repair coverage, standalone AIS and aliases.
- 4A: standard through 250; technical multispeed 100–250; senior 4A280/315/355 base with 34 models across 2/4/6/8/10/12P. `4A_280_ELECTRICAL_SUPPLEMENT.source.json` enriches six clean 280 rows. Detailed Pe1/m1/a1/w1/conductor remains pending row-safe parsing of table 8.21.
- 4AM: senior 4AM280/315/355 base with 34 model/rating identities. `4AM_SENIOR_ELECTRICAL_SUPPLEMENT.source.json` enriches clean 4P cards. Winding remains pending; never copy 4A winding automatically.
- 4AK: phase-rotor 160–250. Ordinary 4AK line ends at 250; do NOT invent 4AK280+.
- 4ANK: phase-rotor 160–355 with rich separate stator/rotor rectangular-conductor data.
- 5A: low/mid/senior incl. 6P/8P/12P supplements. `5A_10P_INDEX_01.source.json` (`6c3c23d`) adds four senior 10P identities: 5AM280S10 37kW/590, 5AM280M10 45kW/590, 5AM315S10 55kW/590, 5AM315M10 75kW/590, all 380/660 and winding_pending. `5A_10P_ELECTRICAL_SUPPLEMENT_01.source.json` (`513cbc5`) merge-enriches them with current/efficiency/power-factor where sourced.
- The complete text extraction of Vitkovoe `5a_3_series.html` was re-audited. Existing 5A base/supplement packages already contained almost all 6P/8P rows. Two genuine senior 8P omissions were added in `5A_8P_SUPPLEMENT_05.source.json` (`99ea02b`): `5AH315S8` (132kW, 155A, 660V, .n4, N24, d1.5, pitch1-8, 2 layers, a4, 530/385/L310/Z72) and `5AH315M8` (160kW, 188A, 660V, .n3, N42, d1.32, pitch1-8, 2 layers, a8, 530/385/L360/Z72).
- Two additional ordinary 8P omissions were added in `5A_8P_SUPPLEMENT_06.source.json` (`9428ca5`): `5A160M8` (11kW, 22.8A, 380V, .n4, N15, d1.18, pitch1-7, 1 layer, a1, 260/180/L195/Z48) and `5A180M8` (15kW, 30.4A, 380V, .n2, N22, d1.25, pitch1-8, 2 layers, a2, 295/209/L195/Z72). Source `.n` is conductor count per turn and is never mapped to parallel strands.
- 6A: IN_PROGRESS; first source-native `6А90В4` has `uп=70`, `w1=420`, `a1=1`, `I=2.0 A`, explicit model-text conflict. Independent 6A 80/90 pages confirm a dedicated 380V star-connected family table, but detailed row values remain image-only/unretrievable. Known `5a6a.htm` remains inaccessible; no inference/OCR guessing.
- A2: IN_PROGRESS. Current directory has two index-only packages plus a selective A2-82-4 geometry supplement. Detailed winding conversion remains a major clean-up target.
- AO2 standard single-speed coverage extends through frames 5–9; main model/rating matrix is substantially closed. `AO2_5_6_WINDING_ENRICHMENT_01.source.json` (`5bb442b`) supplies actual winding data for 10 frame-5/6 cards. `AO2_STANDARD_GAP_WINDING_ENRICHMENT_01.source.json` (`00e4ecf`) is the authoritative gap enrichment for AO2-71-2, 72-2, 82-8 and 91-6.
- AO2/AOL2 multispeed frames 1–9 remain separate in AO_MULTI; P=const/M=const are distinct.
- AOK2: 18 base cards frames 5–7; frame-4 electrical enrichment; frame5 rotor conductor ПЭТВП, frames6–7 ПСД, rotor Y. Section 8.2 was re-read again in this pass; the extracted Z2/y2/Pe2/m2/a2/w2/conductor values remain vertically interleaved across models, so no uncertain detailed rotor row was committed.
- AK2: 12 base cards 81/82/91/92 ×4/6/8P; rotor Y and bare-copper-bar/glass-tape construction merged. Exact bar dimensions and detailed rotor winding remain pending row-safe mapping.
- VAO high-frame: VAO450 rectangular stator, VAO355 full/partial stator enrichments, VAO315 S/M 2P/4P partial enrichment. OCR-ambiguous VAO315 6/8/10 identities remain excluded.
- 4ANK/VAO provide substantial actual rectangular-conductor coverage; dedicated `RECTANGULAR_LV` remains PLANNED because the available Vitkovoe section is image-based.
- CRANE/LIFT/SINGLE_PHASE/GENERATORS/IMPORT/REPAIR/HV/DC/legacy A-AD-AL-AOL retained; avoid duplicates.

## Recent important commits
- `30da70a` preserve `winding_sets_source`; `8f4b4b1` strict merge_only; `6f81a14` merge-aware checker.
- `415866a` senior 4A280–355 base; `421e55c` 4A280 electrical enrichment.
- `dd3f910` senior 4AM280–355 base; `2ac707d` senior 4AM electrical enrichment.
- `2df1c43`, `b6e4f63`, `7a687bd` VAO high-frame enrichments.
- `cf9526c`, `3891489`, `ada97bf`, `d5a0197`, `5f6b8fd`, `f2b44b6`, `a987b13`, `16bc71c`, `3d74222`, `1f51cc7` standard AO2 frames 5–9 expansion.
- `5bb442b` AO2 frame-5/6 merge-only winding enrichment; `00e4ecf` authoritative AO2 gap winding enrichment.
- `621f79a`, `428ba8e` removed branch-race duplicate AO2 sources; `498cb6d`, `7b280f1` removed later redundant AO2 geometry supplements.
- `6c3c23d` senior 5A 10P base (+4); `513cbc5` electrical merge enrichment.
- `99ea02b` missing senior 5A 8P winding rows (+2).
- `9428ca5` missing 5A160M8/5A180M8 winding rows (+2).
- `52002a3` created this live checkpoint.

## Confirmed workflow checkpoints
- `99ea02b`: Motor reference index success; CMP Protocol Tests success.
- `9428ca5`: Motor reference index success; CMP Protocol Tests success.
- Earlier confirmed source/generator commits remain as recorded in history.
- Do not generalize these results to all project CI.

## Exact next continuation point
1. A2 winding enrichment: start with A2-61-2/4/6 and add only row-safe Z/pitch/type/Pe/m/a/w/conductor values from section 8.1; keep any interleaved columns pending.
2. AOK2/AK2 detailed rotor winding: continue seeking an independent row-safe copy of section 8.2 for Z2/y2/Pe2/m2/a2/w2/conductor dimensions/mass/resistance.
3. AO2 frames 7–9 detailed winding enrichment: recover Pe1/m1/a1/w1/conductor only where row-to-column mapping is independently clear. Keep AO2-82-8 725/735 conflict unresolved.
4. Keep probing textual/mirrored 6A80/90 winding data; no OCR guessing or analog transfer.
5. Senior 4A table 8.21 and direct 4AM winding sources remain pending independent row-safe copies.
6. Continue VAO enrichment and retry dedicated rectangular-LV if a textual mirror appears.

## Handoff maintenance rule
After every significant source package, generator/checker change, or new confirmed generated count: fetch this file from `cmp-protocol-v1`, use its current blob SHA, update count/commits/workflow status, and record the exact next continuation target.
