# Motor Reference Live Checkpoint

Updated: 2026-08-17 14:18+06
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
- `firmware/esp32/web/reference/motor-reference.json`: **1401 records**
- `reference_only=true`
- Count is merge-aware.
- 1380 -> 1382: `1f51cc7`, +2 AO2 standard rating gaps.
- 1382 -> 1386: `6c3c23d`, +4 senior 5AM 10-pole base cards.
- 1386 -> 1388: `99ea02b`, +2 missing senior 5AH315 8-pole winding cards.
- 1388 -> 1390: `9428ca5`, +2 missing 5A160M8 / 5A180M8 winding cards.
- 1390 -> 1401: `8448461`, +11 missing standard A2 base cards.
- `5bb442b`, `00e4ecf`, `513cbc5`, `c34ef14`, `32cc77b`, `4827a72` are merge-only enrichments and do not increase count.
- Previous count history and duplicate-cleanup commits remain recorded in git history; no duplicate AO2 supplements from the branch race remain authoritative.

## Architecture already in place
- `tools/build_motor_reference.py`: aliases, spaced `model / alias`, `winding_sets_source`, strict `merge_only`.
- `tools/check_motor_reference.py`: merge-aware and validates supplement provenance via `source_files`.
- Desktop/mobile reference search includes aliases.

## Major coverage state
- AIR/AIS: serial/repair coverage, standalone AIS and aliases.
- 4A: standard through 250; technical multispeed 100–250; senior 4A280/315/355 base with 34 models across 2/4/6/8/10/12P. Senior detailed Pe1/m1/a1/w1/conductor remains pending row-safe independent extraction.
- 4AM: senior 4AM280/315/355 base with 34 model/rating identities plus selective electrical enrichment. Winding remains pending; never copy 4A winding automatically.
- 4AK: phase-rotor 160–250. Do not invent 4AK280+.
- 4ANK: phase-rotor 160–355 with rich separate stator/rotor rectangular-conductor data.
- 5A: low/mid/senior 6P/8P/10P/12P coverage. Four senior 10P cards are winding_pending. Re-audit of text-extractable `5a_3_series.html` found four real 8P omissions, now captured by `99ea02b` and `9428ca5`. Source `.n` is conductor count per turn, not parallel strands; source `a` is winding parallel branches.
- 6A: IN_PROGRESS. Only `6А90В4` has source-native numerical winding data so far; dedicated 80/90 family pages are confirmed but current detailed rows remain image-only/unretrievable. No OCR guessing or analog transfer.
- A2: **standard model coverage substantially completed through frames 6–9**. Existing `A2_INDEX_01/02` were missing 11 standard identities; `A2_INDEX_03.source.json` (`8448461`) adds `А2-71-2`, `А2-92-2`, `А2-91-4`, `А2-62-6`, `А2-71-6`, `А2-72-6`, `А2-61-8`, `А2-62-8`, `А2-81-8`, `А2-82-8`, `А2-92-8`. Only model/power/poles are stored where the independent table is clean; no rpm derived from slip.
- A2 row-safe winding/geometry enrichment now includes:
  - `A2_82_4_GEOMETRY_SUPPLEMENT.source.json` existing partial geometry;
  - `A2_61_6_WINDING_GEOMETRY_SUPPLEMENT.source.json` (`c34ef14`): 291/180, L135, gap0.55, Z36, pitch1-8, two-layer;
  - `A2_62_6_WINDING_GEOMETRY_SUPPLEMENT.source.json` (`32cc77b`): 291/206, L150, gap0.4, Z54, pitch1-8, two-layer;
  - `A2_62_8_WINDING_GEOMETRY_SUPPLEMENT.source.json` (`4827a72`): 291/206, L165, gap0.4, Z54, pitch1-7, two-layer.
  Pe1/m1/a1/w1/conductor remain omitted when the extracted table interleaves neighboring variants.
- AO2 standard single-speed coverage extends through frames 5–9; main model/rating matrix is substantially closed. `5bb442b` supplies actual winding data for 10 frame-5/6 cards; `00e4ecf` is the authoritative gap winding enrichment for AO2-71-2, 72-2, 82-8 and 91-6.
- AO2/AOL2 multispeed frames 1–9 remain separate in AO_MULTI; P=const/M=const remain distinct.
- AOK2: frames 5–7 base cards plus construction/electrical enrichments. Detailed rotor Z2/y2/Pe2/m2/a2/w2/conductor remains pending because current section 8.2 extraction is vertically interleaved.
- AK2: 12 base cards 81/82/91/92 ×4/6/8P; rotor Y and bare-copper-bar/glass-tape construction merged. Exact rotor dimensions/winding remain pending row-safe mapping.
- VAO high-frame: VAO450 rectangular stator, VAO355 full/partial stator enrichments, VAO315 S/M 2P/4P partial enrichment. OCR-ambiguous VAO315 6/8/10 identities remain excluded.
- Dedicated `RECTANGULAR_LV` remains PLANNED because the current Vitkovoe section is image-based, though 4ANK/VAO already provide substantial actual rectangular-conductor examples.

## Recent important commits
- `6c3c23d` senior 5A 10P base (+4); `513cbc5` electrical merge enrichment.
- `99ea02b` missing senior 5A 8P winding rows (+2).
- `9428ca5` missing 5A160M8/5A180M8 winding rows (+2).
- `c34ef14` A2-61-6 partial winding geometry enrichment.
- `8448461` complete missing A2 standard model index (+11).
- `32cc77b` A2-62-6 partial winding geometry enrichment.
- `4827a72` A2-62-8 partial winding geometry enrichment.
- `e0148bf` previous live checkpoint at 1390.

## Confirmed workflow checkpoints
- `99ea02b`, `9428ca5`, `c34ef14`, `8448461`, `32cc77b`, `4827a72`: `Motor reference index` success and `CMP Protocol Tests` success.
- Generated JSON after the A2 block is confirmed at **1401 records**.
- Do not generalize these results to all project CI.

## Exact next continuation point
1. Continue A2 row-safe enrichment: `А2-71-6`, `А2-72-6`, then `А2-81-8`, `А2-82-8`, `А2-92-8`; commit only contiguous Dc/dc/L/gap/Z/pitch/type fields.
2. Seek row-safe Pe1/m1/a1/w1/conductor copies for A2 and AO2. Do not infer from neighboring rows.
3. AOK2/AK2 detailed rotor winding remains a high-value target, but only with an independent row-safe copy of section 8.2.
4. Keep probing textual/mirrored 6A80/90 winding data; no OCR guessing.
5. Senior 4A table 8.21 and direct 4AM winding sources remain pending independent clean copies.
6. Continue VAO enrichment and retry dedicated rectangular-LV if a textual mirror appears.

## Handoff maintenance rule
After every significant source package, generator/checker change, or new confirmed generated count: fetch this file from `cmp-protocol-v1`, use its current blob SHA, update count/commits/workflow status, and record the exact next continuation target.
