# Motor Reference Live Checkpoint

Updated: 2026-08-17 14:06+06
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
- `firmware/esp32/web/reference/motor-reference.json`: **1386 records**
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
- `5bb442b`, `00e4ecf`, `513cbc5` are merge-only enrichments and do not increase count.
- A transient branch race created four duplicate AO2 cards via `66563b8` and `dbc4ec8`; those duplicate source files were removed by `621f79a` and `428ba8e` and canonical count returned to 1382 before the legitimate +4 senior 5A 10P block.
- Two later redundant AO2 geometry supplements created while the branch was changing concurrently were removed by `498cb6d` and `7b280f1`; authoritative AO2 gap enrichment remains `00e4ecf`.

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
- 5A: low/mid/senior incl. 6P/8P/12P supplements. `5A_10P_INDEX_01.source.json` (`6c3c23d`) adds four senior 10P identities: 5AM280S10 37kW/590, 5AM280M10 45kW/590, 5AM315S10 55kW/590, 5AM315M10 75kW/590, all 380/660 and winding_pending. `5A_10P_ELECTRICAL_SUPPLEMENT_01.source.json` (`513cbc5`) merge-enriches those four with current/efficiency/power-factor where explicitly sourced. Vitkovoe `5a_3_series.html` remains a strong text source for senior winding rows; source `.n` means conductor count per turn, not parallel branches.
- 6A: IN_PROGRESS; first source-native `6А90В4` has `uп=70`, `w1=420`, `a1=1`, `I=2.0 A`, explicit model-text conflict. Independent 6A 80/90 pages confirm a dedicated 380V star-connected family table, but detailed row values remain image-only/unretrievable in the current pass. `https://ruslankhashhenko.narod.ru/5a6a.htm` is a known 5A/6A link target but its content is still unavailable; do not infer remaining rows.
- A2: IN_PROGRESS index/reference + selective merge-only enrichment.
- AO2 standard single-speed coverage now extends through frames 5–9 and the major model/rating matrix is substantially closed. Recent base/gap-fill sources:
  - `AO2_5_STANDARD_TECHNICAL_01.source.json` (`cf9526c`): AO2-51-4, AO2-51-6.
  - `AO2_5_STANDARD_TECHNICAL_02.source.json` (`3891489`): AO2-52-4/6/8.
  - `AO2_6_STANDARD_TECHNICAL_01.source.json` (`ada97bf`): AO2-61-6, AO2-62-4/6.
  - `AO2_7_STANDARD_TECHNICAL_01.source.json` (`d5a0197`): AO2-71/72-6 and AO2-71/72-8.
  - `AO2_8_9_STANDARD_TECHNICAL_01.source.json` (`5f6b8fd`): AO2-81/82-6, AO2-81-8, AO2-91/92-4, AO2-92-6, AO2-91-8.
  - `AO2_8_9_STANDARD_TECHNICAL_02.source.json` (`f2b44b6`): AO2-81/82-2, AO2-81-4, AO2-91/92-2, AO2-92-8, AO2-91/92-10; AO2-92-2 current `312/108` preserved literally and flagged suspect.
  - `AO2_7_STANDARD_TECHNICAL_02.source.json` (`a987b13`): AO2-71-4 and AO2-72-4.
  - `AO2_STANDARD_GAP_FILL_01.source.json` (`16bc71c`): AO2-71-2, AO2-72-2, AO2-82-8, AO2-91-6.
  - `AO2_STANDARD_GAP_FILL_02.source.json` (`3d74222`): AO2-51-2, AO2-52-2, AO2-51-8, AO2-61-4, AO2-61-8, AO2-62-8, AO2-81-10, AO2-82-10.
  - `AO2_STANDARD_GAP_FILL_03.source.json` (`1f51cc7`): AO2-62-2 and AO2-82-4.
- `AO2_5_6_WINDING_ENRICHMENT_01.source.json` (`5bb442b`) merge-enriches 10 AO2 frame-5/6 cards with actual winding fields from the Electroceh text table: Z, coil/group data, pitch, parallel branches, source-native turns, wire, copper mass, bore/core length and Y. Compound N and mixed conductor forms remain literal/review-required.
- `AO2_STANDARD_GAP_WINDING_ENRICHMENT_01.source.json` (`00e4ecf`) is the authoritative merge-only enrichment for the four gap-fill bases: AO2-71-2 gets 72.8/42.1 A, 343/183, L115, Z36, pitch 1-12, two-layer; AO2-72-2 gets safe 343/183 geometry; AO2-82-8 gets current 104/60.2 while retaining base 735 rpm because Likhachev exposes a 725/735 conflict; AO2-91-6 gets 169/98 A, 458/334, L240, Z72, pitch 1-11, two-layer. Detailed Pe1/m1/a1/w1/conductor remains pending where row ordering interleaves neighboring variants.
- AO2/AOL2 multispeed frames 1–9 already tracked separately in AO_MULTI; P=const/M=const remain distinct.
- AOK2: 18 base cards frames 5–7; frame-4 electrical enrichment; frame5 rotor conductor ПЭТВП, frames6–7 ПСД, rotor Y. Detailed rotor Z2/y2/Pe2/m2/a2/w2/conductor dimensions/mass/resistance remain pending row-safe section 8.2 extraction.
- AK2: 12 base cards 81/82/91/92 ×4/6/8P; rotor Y and bare-copper-bar/glass-tape construction merged. Exact bar dimensions and detailed rotor winding remain pending row-safe mapping.
- VAO high-frame: VAO450 rectangular stator, VAO355 full/partial stator enrichments, VAO315 S/M 2P/4P partial enrichment. OCR-ambiguous VAO315 6/8/10 identities remain excluded.
- 4ANK/VAO provide substantial actual rectangular-conductor coverage; dedicated `RECTANGULAR_LV` remains PLANNED because Vitkovoe section 18 is image-based in current retrieval.
- CRANE/LIFT/SINGLE_PHASE/GENERATORS/IMPORT/REPAIR/HV/DC/legacy A-AD-AL-AOL retained; avoid duplicates.

## Recent important commits
- `30da70a` preserve `winding_sets_source`; `8f4b4b1` strict merge_only; `6f81a14` merge-aware checker.
- `415866a` senior 4A280–355 base; `421e55c` 4A280 electrical enrichment.
- `dd3f910` senior 4AM280–355 base; `2ac707d` senior 4AM electrical enrichment.
- `2df1c43`, `b6e4f63`, `7a687bd` VAO high-frame enrichments.
- `cf9526c`, `3891489`, `ada97bf`, `d5a0197`, `5f6b8fd`, `f2b44b6`, `a987b13`, `16bc71c`, `3d74222`, `1f51cc7` standard AO2 frames 5–9 expansion.
- `5bb442b` AO2 frame-5/6 merge-only winding enrichment.
- `00e4ecf` authoritative AO2 standard gap merge-only winding enrichment.
- `621f79a`, `428ba8e` removed duplicate AO2 source files created during a concurrent branch race.
- `498cb6d`, `7b280f1` removed later redundant AO2 geometry supplements after detecting overlap with `00e4ecf`.
- `6c3c23d` senior 5A 10P base (+4); `513cbc5` electrical merge-only enrichment.
- `52002a3` created this live checkpoint.

## Confirmed workflow checkpoints
- `7a687bd`, `5f6b8fd`, `f2b44b6`, `a987b13`, `16bc71c`, `3d74222`, `1f51cc7`, `5bb442b`, `00e4ecf`: Motor reference index success and CMP Protocol Tests success.
- The temporary `f52b278` and `442bd00` supplements each passed both specialized workflows before being removed as redundant; their removal does not remove any authoritative data because `00e4ecf` already contained the same or richer fields.
- Do not generalize these results to all project CI.

## Exact next continuation point
1. AOK2/AK2 detailed rotor winding: recover row-safe Z2/y2/Pe2/m2/a2/w2/conductor dimensions/mass/resistance from section 8.2; construction/Y are already merged. Start with AOK2 frames 5–7 and only commit contiguous model-to-row mappings.
2. AO2 frames 7–9 detailed winding enrichment: recover Pe1/m1/a1/w1/conductor only where row-to-column mapping is independently clear. Do not overwrite AO2-82-8 735 rpm until the 725/735 source conflict is resolved.
3. Keep probing textual/mirrored 6A80/90 winding data, including the known but currently unretrievable `5a6a.htm`; no OCR guessing or analog transfer.
4. Senior 4A table 8.21: seek independent copies for Pe1/m1/a1/w1/conductor before writing winding columns.
5. 4AM direct winding sources only; no automatic transfer from 4A.
6. Continue VAO enrichment and retry rectangular-LV section 18 if a textual mirror appears.

## Handoff maintenance rule
After every significant source package, generator/checker change, or new confirmed generated count: fetch this file from `cmp-protocol-v1`, use its current blob SHA, update count/commits/workflow status, and record the exact next continuation target.
