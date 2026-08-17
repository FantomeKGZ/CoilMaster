# Phase-rotor 4AK reference expansion — 2026-08-17

Branch: `cmp-protocol-v1`

## Scope

This checkpoint records the new read-only 4AK / phase-rotor reference layer. It does not change CoilMaster production motor data, physical START behavior, SSR control, reboot behavior, or wire write-off semantics.

## New source packages

```text
data/motor_catalog/4AK/4AK_160.source.json
data/motor_catalog/4AK/4AK_180_200.source.json
data/motor_catalog/4AK/4AK_225.source.json
data/motor_catalog/4AK/4AK_250.source.json
```

Commits:

```text
df2a63f45e18d4c97982e9a086017640f4b87681  4AK160
b5ccde2add0fec81f87eb0bcdf4731a71b0d3303  4AK180-200
a752cd5d9275c20d88b488c7122655622fd18d53  4AK225
968b8a59a18c9867634ea397a78601186c992eb9  4AK250
95d5753af7fe4e3f3f4b27dbbe9b4e62bf39401e  catalog 4AK coverage
```

## Source semantics

Primary winding-data source: table 6.15, asynchronous motors with phase rotor, IP44.

Important fields are preserved source-native:

- `Sп` is stored as `source_sp_value` and is not mapped directly to `coil_program`;
- the printed `n/a` fraction is stored as `source_n_over_a_text` and is not mapped automatically to CoilMaster parallel strands;
- `d/d′` is stored as source conductor text;
- stator pitch remains `winding_pitch_source`;
- phase-rotor winding data remain separate from stator winding data;
- rectangular rotor conductor notation such as `2.26×16.8` is not treated as a round-wire diameter.

The web transcription of the printed table wraps some fractional and voltage-dependent values across lines. Those values are retained as literal source text and marked review-required instead of being guessed into normalized winding fields.

## Model-label cross-check

The 250-frame table transcription contains OCR artifacts. Independent technical-reference rows were used only to resolve model labels, including:

```text
4АК250SА4У3
4АК250SВ4У3
4АК250М4У3
4АК250S6У3
4АК250М6У3
4АК250S8У3
4АК250М8У3
```

This prevents erroneous labels such as `4AK250SB4V3` or `4АК260S6У3` from becoming canonical CoilMaster reference model names.

## Generated reference checkpoint

After the 4AK source additions, the generated static reference reached:

```text
record_count = 982
reference_only = true
```

The increase from the previous 958 checkpoint is exactly 24 new phase-rotor reference cards:

- 6 × 4AK160;
- 8 × 4AK180/200;
- 3 × 4AK225;
- 7 × 4AK250.

## Coverage register

`catalog.json` now contains a dedicated entry:

```text
4AK / 4АНК с фазным ротором — IN_PROGRESS
```

4AK remains separate from ordinary `4A / 4АН` because phase-rotor winding construction and source semantics are materially different.

## Next work

1. Extend 4AK/4ANK only when stator/rotor table layout is sufficiently unambiguous.
2. Continue searching for complete 4AM/4AI/4AIM winding-data tables.
3. Add 6A only from real winding rows containing `N`, conductor and pitch data.
4. Capture the dedicated low-voltage rectangular-wire table when the source rows become safely extractable.
5. Keep 4AK source data reference-only; no `coil_program` promotion without an explicit reviewed conversion rule.

## Safety boundary unchanged

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically deducts wire;
- wire write-off remains manual and tied to exact `spool_id + source_session_id + source_run_id`.
