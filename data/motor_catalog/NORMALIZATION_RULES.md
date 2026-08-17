# Motor catalogue normalization rules

This document defines the repository-side conversion from source-native winding notation to CoilMaster import fields.

The raw `*.source.json` files remain authoritative transcriptions of their cited sources. Import-ready `*.json` files may contain derived values only when the derivation is deterministic and documented here.

## Provenance rule

If `coil_program`, `turns_per_coil`, pole count, strand count, or another essential winding field is derived rather than copied literally, the import record must use:

- `calculated_fields: true`;
- `confidence: "CALCULATED"`;
- the original source type in `source_type` when the source itself is still accurately described as `TECHNICAL_REFERENCE` or `REPAIR_RECORD`.

Do not upgrade derived data to `VERIFIED` or `CORROBORATED` merely because the source table is widely copied.

## Source notation used by the AIR/AIS reference table family

The historical notation accompanying this table family defines:

- `y = 11;9;7` as a single-layer concentric winding with nested coils; the listed pitches correspond to separate coils in one concentric coil group;
- a plain source `N` value as the effective conductors/turns occupying the slot for the single-layer coil side;
- `38 × 2` as a single-layer winding with 38 effective conductors/turns, wound with two parallel wires in hand;
- `(11+11)3` as a two-layer winding with 11 effective conductors in each layer, wound with three parallel wires in hand.

The source websites do not all label `N/n` consistently: some say "turns in slot" while reproductions call it "full/effective conductors in slot". Therefore conversion is based on winding construction, not on the column heading alone.

## Safe single-layer concentric mapping

A raw AIR/AIS record may be converted automatically only when all of the following hold:

1. the pitch notation is a simple semicolon-separated list such as `11;9`, `11;9;7`, or `7;5`;
2. the notation source identifies this form as a single-layer concentric group;
3. `N` is a single integer, optionally followed by an explicit `× strand_count` notation;
4. there is no `+`, parenthesized layer notation, slash, or other unresolved construction marker in the pitch/turn notation.

For such a record:

- each listed pitch represents one nested coil in the CoilMaster winding program;
- each coil receives the same turn count `N`;
- `coil_program` is `N` repeated once for each listed pitch;
- `turns_per_coil = N`;
- `parallel_strands` may be populated only when strand count is explicit in the source notation; otherwise it is omitted;
- the complete source pitch list is retained in `comment`, because CoilMaster currently has only one scalar `coil_pitch` field and cannot losslessly store a concentric pitch list.

Examples:

- `N=78`, `y=11;9` -> `coil_program="78/78"`, `turns_per_coil=78`;
- `N=91`, `y=11;9;7` -> `coil_program="91/91/91"`, `turns_per_coil=91`;
- `N=110`, `y=7;5` -> `coil_program="110/110"`, `turns_per_coil=110`.

The program represents one concentric coil group. The number and placement of repeated groups around the stator are not encoded by repeating `coil_program` values beyond the coils that belong to that group.

## Not automatically normalized yet

Keep the record in `*.source.json` only when any of these are present until a separate rule is proven:

- asymmetric or compound pitch notation such as `5;3+5`;
- two-layer notation such as `(11+11)3`;
- mixed turn notation such as `(15+16)2` or `21+21`;
- source geometry that is internally impossible or explicitly marked suspect;
- a repair observation whose layer/construction is not established by the source notation;
- any case where the source only gives conductors-in-slot but coil-side ownership cannot be established.

## Fields that must not be conflated

- source parallel branches `a` are not CoilMaster `parallel_strands`;
- source pitch lists are not losslessly representable by scalar `coil_pitch`;
- a dual-voltage source value such as `220/380` must not be forced into scalar `rated_voltage_v`; retain it in `comment` unless a specific variant voltage is independently established;
- paired source currents such as `3.0/1.7 A` must not be forced into scalar `rated_current_ma` without a known voltage/connection association for the imported variant.

## Review boundary

An import-ready package is allowed to contain only records whose derivation follows a rule in this file. Unsupported source notation must remain staged rather than being guessed into a syntactically valid but technically false CoilMaster record.
