# Motor Reference Live Checkpoint

Updated: 2026-08-18 00:15+06
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
- `firmware/esp32/web/reference/motor-reference.json`: **1426 records**
- `reference_only=true`
- Count is merge-aware.
- 1380 -> 1382: `1f51cc7`, +2 AO2 standard rating gaps.
- 1382 -> 1386: `6c3c23d`, +4 senior 5AM 10-pole base cards.
- 1386 -> 1388: `99ea02b`, +2 missing senior 5AH315 8-pole winding cards.
- 1388 -> 1390: `9428ca5`, +2 missing 5A160M8 / 5A180M8 winding cards.
- 1390 -> 1401: `8448461`, +11 missing standard A2 base cards.
- 1401 -> 1426: `b638de1`, +25 distinct 6000 V VAO450/500/560/630 index cards; `416dd21` then added explicit `...-6000V` variant keys without changing count.
- Recent A2/4A commits are merge-only/electrical enrichment and do not increase count.

## Architecture already in place
- `tools/build_motor_reference.py`: aliases, spaced `model / alias`, `winding_sets_source`, strict `merge_only`.
- `tools/check_motor_reference.py`: merge-aware and validates supplement provenance via `source_files`.
- Desktop/mobile reference search includes aliases.

## Major coverage state
- AIR/AIS: serial/repair coverage, standalone AIS and aliases.
- 4A: standard through 250; technical multispeed 100–250; senior 4A280/315/355 base with 34 models across 2/4/6/8/10/12P. Senior electrical identification has now been extended where table 8.21 is row-safe; detailed Pe1/m1/a1/w1/conductor still requires a clean row-aligned source.
- 4AM: senior 4AM280/315/355 base with 34 model/rating identities plus selective electrical enrichment. Winding remains pending; never copy 4A winding automatically.
- 4AK: phase-rotor 160–250. Do not invent 4AK280+.
- 4ANK: phase-rotor 160–355 with rich separate stator/rotor rectangular-conductor data.
- 5A: low/mid/senior 6P/8P/10P/12P coverage. Four senior 10P cards are winding_pending. Re-audit of text-extractable `5a_3_series.html` found four real 8P omissions, now captured by `99ea02b` and `9428ca5`. Source `.n` is conductor count per turn, not parallel strands; source `a` is winding parallel branches.
- 6A: IN_PROGRESS. Only `6А90В4` has source-native numerical winding data so far; dedicated 80/90 family pages are confirmed but current detailed rows remain image-only/unretrievable. No OCR guessing or analog transfer.
- A2: standard model coverage substantially completed through frames 6–9. `A2_INDEX_03.source.json` (`8448461`) added 11 missing standard identities. `64f8cd2` later enriched exact ETI operating rows without deriving rpm from slip: А2-71-2 2900rpm/56.2A@380V; А2-62-6 965/26.1; А2-71-6 965/33.2; А2-72-6 965/43; А2-61-8 725/17.2; А2-62-8 725/22.1.
- A2 row-safe winding/geometry enrichment includes:
  - `A2_82_4_GEOMETRY_SUPPLEMENT.source.json` existing partial geometry;
  - `A2_61_6_WINDING_GEOMETRY_SUPPLEMENT.source.json` (`c34ef14`): 291/180, L135, gap0.55, Z36, pitch1-8, two-layer;
  - `A2_62_6_WINDING_GEOMETRY_SUPPLEMENT.source.json` (`32cc77b`): 291/206, L150, gap0.4, Z54, pitch1-8, two-layer;
  - `A2_62_8_WINDING_GEOMETRY_SUPPLEMENT.source.json` (`4827a72`): 291/206, L165, gap0.4, Z54, pitch1-7, two-layer;
  - branch also contains row-safe supplements for А2-91-8, А2-92-2 electrical data and А2-92-8 geometry/winding type; do not duplicate them.
- AO2 standard single-speed coverage extends through frames 5–9; main model/rating matrix is substantially closed. `5bb442b` supplies actual winding data for 10 frame-5/6 cards; `00e4ecf` is the authoritative gap winding enrichment for AO2-71-2, 72-2, 82-8 and 91-6.
- AO2/AOL2 multispeed frames 1–9 remain separate in AO_MULTI; P=const/M=const remain distinct. `MIXED_MULTISPEED_06.source.json` was checked against the older mixed files; its АИР80А4/2У3 and ДСХН I-42/8-642 rows are unique, not duplicates.
- REPAIR_RECORDS `motor_series.html` is text-complete against `repair_motor_series_01/02`; no new repair cards were found in the latest audit.
- AOK2: frames 5–7 base cards plus construction/electrical enrichments. Detailed rotor Z2/y2/Pe2/m2/a2/w2/conductor remains pending because current section 8.2 extraction is vertically interleaved.
- AK2: 12 base cards 81/82/91/92 ×4/6/8P; rotor Y and bare-copper-bar/glass-tape construction merged. Exact rotor dimensions/winding remain pending row-safe mapping.
- VAO low/mid/high frame: low-voltage VAO315/355/450 remains covered with index + partial/rich stator supplements; VAO450 includes actual rectangular PSD conductor data for 2/4/6/8/10P.
- `VAO_450_630_6KV_INDEX.source.json` (`b638de1`, corrected by `416dd21`) adds a separate 6000 V high-frame family: 25 cards across 450/500/560/630 frames and 2/4/6/8 poles. Every row has an explicit `...-6000V` variant key. These are `INDEX_ONLY` and winding_pending.
- Never transfer the existing 380/660 V VAO450 stator winding data into the 6000 V variants merely because model names overlap; equivalence has not been proven.
- Dedicated `RECTANGULAR_LV` remains PLANNED because the current Vitkovoe section is image-based, though 4ANK/VAO already provide substantial actual rectangular-conductor examples.

## Senior 4A table 8.21 — safe electrical pass
- `4A_280_ELECTRICAL_SUPPLEMENT.source.json` already covers clean 4A280 S/M rows for 4P, 8P and 10P; 280 2P/6P remain interleaved and excluded.
- `4A_315_ELECTRICAL_SUPPLEMENT_01.source.json` (`2ca998f`) adds clean contiguous rows:
  - 4А315М10: 75kW, 590rpm, 220/380V, 260/150A;
  - 4А315S12: 45kW, 490rpm, 220/380V, 171/99A;
  - 4А315М12: 55kW, 490rpm, 220/380V, 204/118A.
- `4A_355_ELECTRICAL_SUPPLEMENT_01.source.json` created by `085c9d0` and extended by `a13fab6` now contains:
  - 4А355S4: 250kW, 1485rpm, 380/660V, 432/250A;
  - 4А355S10: 90kW, 590rpm, 220/380V, 294/169.5A;
  - 4А355М10: 110kW, 590rpm, 220/380V, 357/206A.
- 4A355 M4, S/M6, S/M8 and 12P remain intentionally omitted because table extraction interleaves 4AN/4AK/4ANK rows.
- Direct 4AM280/315/355 winding search still produced no usable direct winding source; no 4A→4AM transfer is allowed.

## Recent important commits
- `6c3c23d` senior 5A 10P base (+4); `513cbc5` electrical merge enrichment.
- `99ea02b` missing senior 5A 8P winding rows (+2).
- `9428ca5` missing 5A160M8/5A180M8 winding rows (+2).
- `8448461` complete missing A2 standard model index (+11).
- `c34ef14`, `32cc77b`, `4827a72` A2 row-safe winding geometry enrichments.
- `64f8cd2` A2 exact operating-data enrichment from ETI.
- `085c9d0` clean 4A355S4 electrical row.
- `2ca998f` clean 4A315 M10/S12/M12 electrical rows.
- `a13fab6` extends 4A355 supplement with S10/M10.
- `b638de1` adds 25 VAO450–630 6000 V base cards.
- `416dd21` adds explicit 6000 V variant keys to keep same-name low/high-voltage VAO cards separate.

## Confirmed workflow checkpoints
- `64f8cd2`, `085c9d0`, `2ca998f`, `a13fab6`, `416dd21`: `Motor reference index` success and `CMP Protocol Tests` success.
- Earlier recent A2/5A commits recorded above also passed both specialized workflows.
- Generated JSON is now confirmed at **1426 records** after the 25-card 6000 V VAO expansion.
- Do not generalize these results to all project CI.

## Exact next continuation point
1. Seek direct winding sources for VAO500/560/630 6000 V; keep these new cards winding_pending until an exact voltage/model winding table is recovered.
2. Continue VAO/rectangular-conductor enrichment where primary text or a clean image card exposes actual conductor dimensions, turns and slot/pitch data; do not OCR-guess ambiguous model identities.
3. Seek row-safe Pe1/m1/a1/w1/conductor copies for senior 4A/A2/AO2; the electrical-identification pass is near its safe limit.
4. AOK2/AK2 detailed rotor winding remains a high-value target, but only with an independent row-safe copy of section 8.2.
5. Keep probing textual/mirrored 6A80/90 winding data; no analog transfer.
6. Direct 4AM winding sources only; never inherit winding from 4A.
7. Retry dedicated `RECTANGULAR_LV` section if a text mirror or readable source image appears.

## Handoff maintenance rule
After every significant source package, generator/checker change, or new confirmed generated count: fetch this file from `cmp-protocol-v1`, use its current blob SHA, update count/commits/workflow status, and record the exact next continuation target.
