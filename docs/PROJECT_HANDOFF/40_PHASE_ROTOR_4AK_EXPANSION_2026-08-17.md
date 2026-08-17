# Phase-rotor 4AK / 4ANK reference expansion — 2026-08-17

Branch: `cmp-protocol-v1`

## Scope

This checkpoint records the read-only 4AK / 4ANK phase-rotor reference layer. It does not change CoilMaster production motor data, physical START behavior, SSR control, reboot behavior, or wire write-off semantics.

## 4AK source packages

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
```

Primary 4AK winding-data source is table 6.15, phase-rotor motors with IP44.

## 4ANK source packages

```text
data/motor_catalog/4ANK/4ANK_160_180.source.json
data/motor_catalog/4ANK/4ANK_200_225.source.json
data/motor_catalog/4ANK/4ANK_250_4P_6P.source.json
```

Commits:

```text
18456415ceb2179f288c14754814e89f89346469  4ANK160-180
d27544caeccef9cedddce0f8324c1f767f4b1bfc  4ANK200-225
4258575ef9ab850769c17b4cc9a804ddebd55101  4ANK250 4P/6P
61d6853f6c2124e5a24c5c077b4f963b37f7ba8a  split catalog coverage
```

Primary 4ANK winding-data source is table 6.16, phase-rotor motors with IP23.

The 4ANK source HTML alternates OCR forms such as `4AHK` and `4НK`; the catalogue stores the canonical Cyrillic `4АНК` model form. Only model labels are normalized. Winding values remain source-native.

## Source semantics

For both 4AK and 4ANK:

- stator and phase-rotor data remain separate;
- `Sп` is preserved as source-native conductor-side data and is not mapped directly to `coil_program`;
- printed `n/a` fractions remain literal and are not mapped automatically to CoilMaster `parallel_strands`;
- `d/d′` is retained as source conductor text;
- source pitch remains literal;
- rectangular rotor bars/conductors are never interpreted as round-wire diameters;
- OCR-suspect values are retained literally and flagged for review instead of silently corrected.

The 4ANK225 and 4ANK250 phase rotors explicitly contain rectangular source conductor notation such as:

```text
(2.26×16.8)/(3.26×17.8)
(2.44×16.8)/(3.44×17.8)
```

These are phase-rotor conductor dimensions, not CoilMaster round-wire diameters.

## 4ANK coverage captured

Current source packages contain:

- 4ANK160S/M, 4 poles;
- 4ANK180S/M, 4/6/8 poles;
- 4ANK200M/L, 4/6/8 poles;
- 4ANK225M, 4/6/8 poles;
- 4ANK250SA/SB/M, 4/6 poles.

The 250-frame 8-pole rows are deliberately deferred because the HTML transcription alternates `SA8`, `SB8` and `S8` labels. No canonical model is guessed from that ambiguous transcription.

## Generated reference checkpoint

After 4AK, the static reference reached 982 records.

After completed 4ANK160-180 and 4ANK200-225 rebuilds, the confirmed generated checkpoint is:

```text
record_count = 999
reference_only = true
```

This is a generated-reference count, not a VERIFIED production-motor count.

The 4ANK250 4P/6P source package adds six more source records, but the confirmed static JSON checkpoint above was read before that source commit's regeneration had been observed. Do not claim the higher count until a fresh generated-index fetch confirms it.

## Coverage register

`catalog.json` now tracks the two phase-rotor families separately:

```text
4AK  — PHASE_ROTOR — IN_PROGRESS
4ANK — PHASE_ROTOR — IN_PROGRESS
```

They remain separate from ordinary `4A / 4АН` because the phase-rotor construction and source semantics are materially different.

## Next work

1. Recheck the generated index after the 4ANK250 rebuild and confirm the new exact count.
2. Resolve 4ANK250 8-pole SA/SB/S source labels before adding those rows.
3. Continue 4ANK280 only where every stator/rotor line can be bound unambiguously to a model.
4. Continue searching for complete 4AM/4AI/4AIM winding-data tables.
5. Add 6A only from real winding rows containing `N`, conductor and pitch data.
6. Keep dedicated rectangular-wire work distinct from rectangular phase-rotor conductors already captured here.
7. Keep phase-rotor data reference-only; no `coil_program` promotion without an explicit reviewed conversion rule.

## Safety boundary unchanged

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically deducts wire;
- wire write-off remains manual and tied to exact `spool_id + source_session_id + source_run_id`.
