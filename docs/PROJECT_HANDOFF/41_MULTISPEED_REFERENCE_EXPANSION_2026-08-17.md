# Multispeed reference expansion checkpoint — 2026-08-17

Branch: `cmp-protocol-v1`

## Current generated reference

The merge-aware static winding reference currently contains:

```text
record_count = 1224
reference_only = true
```

This count is a reference-card count, not a VERIFIED production motor count.

## New multispeed technical coverage

The reference now includes a continuous technical-reference layer for many Soviet multispeed families in addition to the earlier repair/reference observations.

### AO2 / AOL2

Technical index coverage from Likhachev tables 8.35–8.42 now spans 1st through 9th frames and preserves distinct physical/operating variants such as:

- 4/2;
- 6/4 with separate P=const and M=const variants where the handbook distinguishes them;
- 8/4;
- 12/6;
- 6/4/2;
- 8/6/4;
- 12/8/6/4.

Same pole labels with different handbook operating semantics remain separate records through `variant_key`.

### 4A technical multispeed

New technical source packages:

```text
data/motor_catalog/AO_MULTI/4A132_TECHNICAL_MULTISPEED_01.source.json
data/motor_catalog/AO_MULTI/4A_TECHNICAL_MULTISPEED_02.source.json
data/motor_catalog/AO_MULTI/4A_TECHNICAL_MULTISPEED_03.source.json
```

Coverage now includes confirmed 4A100/112/132/160/180/200/225/250 multispeed designs with 4/2, 6/4/2, 8/4/2, 8/6/4, 8/6/4/2 and 12/8/6/4 pole combinations.

For higher-frame four-speed machines, source-native pole-changing winding groups, pitch and unambiguous Sп values are retained separately in `winding_sets_source`. Compound values are never converted automatically into CoilMaster `coil_program`.

## Merge-aware enrichment contract

`tools/build_motor_reference.py` now supports `merge_only` supplementary source documents. These enrich one and only one existing reference card instead of inflating `record_count`.

A merge-only record must resolve unambiguously to its base card. Failure to find exactly one target is a build error.

`tools/check_motor_reference.py` uses the same merge-aware expected-record semantics, so freshness validation does not reintroduce supplementary duplicates.

The generator also preserves both `winding_sets` and source-native `winding_sets_source`, allowing phase-rotor and multispeed source detail to reach the static reference without mapping source `n/a` into CoilMaster parallel strands.

## Other current expansion layers

Current active reference coverage also includes:

- 4AK and 4ANK phase-rotor families;
- standalone AIS plus AIR/AIS aliases;
- 5A 6/8/12-pole technical rows;
- lift motors with separate speed winding sets;
- VAO explosion-protected index/reference data including high-frame rectangular-wire stator enrichment;
- A2 index-only technical coverage where winding-column alignment is not yet reliable;
- repair, import, generator, single-phase, crane, DC and high-voltage reference layers.

## Safety boundary unchanged

Nothing in this reference layer changes production control:

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically deducts wire;
- wire write-off remains manual and tied to exact `spool_id + source_session_id + source_run_id`;
- reference rows never automatically enter the verified production motor database.

## Next priorities

1. Continue technical multispeed 4A tables beyond the newly captured 100–250 frame rows where source layout is unambiguous.
2. Expand VAO multispeed/reference winding data using merge-only enrichment rather than duplicate cards.
3. Enrich AO2/AOL2 index records with winding fields only where handbook row-to-column mapping is unambiguous.
4. Continue A2 winding extraction while retaining INDEX_ONLY status for unresolved rows.
5. Keep searching for actual 6A winding tables and dedicated low-voltage rectangular-wire model tables; do not synthesize them from dimensional catalogues.
