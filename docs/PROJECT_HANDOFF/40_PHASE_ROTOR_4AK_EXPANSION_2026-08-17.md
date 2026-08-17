# Phase-rotor 4AK / 4ANK reference expansion — 2026-08-17

Branch: `cmp-protocol-v1`

## Scope

This checkpoint records the read-only 4AK / 4ANK phase-rotor reference layer. It does not change CoilMaster production motor data, physical START behavior, SSR control, reboot behavior, or wire write-off semantics.

## Coverage summary

4AK remains a separate IP44 phase-rotor source family; 4ANK is tracked separately as the IP23 phase-rotor family.

Current phase-rotor source coverage includes 4AK 160–250 and 4ANK 160–355 reference rows, with stator and rotor data preserved separately whenever the source exposes both blocks.

## Source semantics

For both 4AK and 4ANK:

- stator and phase-rotor data remain separate;
- `Sп` stays source-native and is never mapped directly to `coil_program`;
- printed `n/a` or `m/a` fractions remain literal and are not mapped automatically to CoilMaster `parallel_strands`;
- `d/d′` and rectangular `a×b/A×B` conductor notation remain source text;
- OCR-suspect values are retained literally and marked review-required instead of silently corrected;
- no reference row is promoted automatically into the production winding database.

## 4ANK source packages

```text
data/motor_catalog/4ANK/4ANK_160_180.source.json
data/motor_catalog/4ANK/4ANK_200_225.source.json
data/motor_catalog/4ANK/4ANK_200_250.source.json
data/motor_catalog/4ANK/4ANK_250_4P_6P.source.json
data/motor_catalog/4ANK/4ANK_280_4P.source.json
data/motor_catalog/4ANK/4ANK_315.source.json
data/motor_catalog/4ANK/4ANK_355.source.json
```

Notable commits:

```text
18456415ceb2179f288c14754814e89f89346469  4ANK160-180
d27544caeccef9cedddce0f8324c1f767f4b1bfc  4ANK200-225
4258575ef9ab850769c17b4cc9a804ddebd55101  4ANK250 4P/6P
a9c9379d227b4a6bd2c043fd711c217845876cb8  4ANK280 4P rectangular stator+rotor
99c9e089c257d8526c532862cc559a4b4d059038  4ANK355
f885e151c7ffaa3d0b3bbb13cc3e1589b59688cf  deduplicate overlapping 4ANK200-250 package
61d6853f6c2124e5a24c5c077b4f963b37f7ba8a  split 4AK/4ANK catalog coverage
```

The 4ANK315 source also contains completed stator winding sets for 10/12-pole rows in addition to their phase-rotor data.

## Deduplication

A concurrently added `4ANK_200_250.source.json` overlapped the richer `4ANK_200_225.source.json` and `4ANK_250_4P_6P.source.json` packages.

The overlap was audited and removed. Fifteen duplicate records were deleted from the overlapping file while three unique 250-frame 8-pole stator records were retained. The retained 8-pole rows remain rotor-pending because the HTML transcription alternates SA/SB/S labels and no rotor model binding is guessed.

## Rectangular conductor coverage

The phase-rotor layer now contains real rectangular conductor data, including:

```text
(2.26×16.8)/(3.26×17.8)
(2.44×16.8)/(3.44×17.8)
(3.05×18)/(4.3×19.2)
```

4ANK280S4/M4 are especially important because rectangular conductor notation is present in both the stator and the phase rotor. These values remain reference-only and are not treated as round-wire diameters.

## Generated reference checkpoint

After the deduplication rebuild and the concurrently completed 4ANK315/355 source additions, the generated static reference is confirmed at:

```text
record_count = 1045
reference_only = true
```

This is a generated-reference count, not a VERIFIED production-motor count.

## Coverage register

`catalog.json` tracks the two phase-rotor families separately:

```text
4AK  — PHASE_ROTOR — IN_PROGRESS
4ANK — PHASE_ROTOR — IN_PROGRESS
```

They remain separate from ordinary `4A / 4АН`.

## Next work

1. Audit remaining 4ANK250/280 8/10/12-pole rows where model binding or multiple voltage/conductor variants are still ambiguous.
2. Continue searching for complete 4AM/4AI/4AIM winding-data tables.
3. Add 6A only from real winding rows containing `N`, conductor and pitch data.
4. Keep dedicated rectangular-wire coverage distinct from phase-rotor rectangular-conductor observations.
5. Continue detailed DC records only when source cards can be read reliably.
6. Keep phase-rotor data reference-only; no `coil_program` promotion without an explicit reviewed conversion rule.

## Safety boundary unchanged

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically deducts wire;
- wire write-off remains manual and tied to exact `spool_id + source_session_id + source_run_id`.
