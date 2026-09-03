# Checkpoint 20 — branch policy and Arduino Uno headroom

Date: 2026-09-03

## Current branch policy

This checkpoint supersedes older PROJECT_HANDOFF branch-policy sections that still name `arduino-ru-lcd-experiment` as the working branch.

Current policy is:

- `main` — ready/stable/finished state only;
- `cmp-protocol-v1` — the only active development/source branch;
- `arduino-ru-lcd-experiment` — retired from active development after its complete state was synchronized; do not use it for new work;
- `main` must not be used as the development source.

The synchronized base before subsequent Uno optimization work was:

```text
d5db7a095d5965306940749d140ef49c49d2955f
```

All work described below was performed only in `cmp-protocol-v1`.

## Arduino Uno Flash-headroom optimization

The optimization block was intentionally limited to build/code-size changes that preserve runtime behavior, CMP protocol semantics and hardware safety invariants.

### Constant merging

Commit:

```text
39a073c733a5d48cebc1cd25be4566f60d45c263
build(uno): test constant merging
```

Production `uno` gained `-fmerge-all-constants`, already used by the RU LCD experimental configuration.

Exact verification:

```text
Arduino Uno Build #255
run 33726680698
completed/success

CMP Protocol Tests #4819
run 33726680713
completed/success

ESP32 Build #1870
run 33726680842
completed/success
```

Measured Uno resources:

```text
RAM:   1227 / 2048 bytes; 821 bytes free
Flash: 31206 / 32256 bytes; 1050 bytes free
```

A preceding experiment with `-fno-rtti` and `-fno-threadsafe-statics` in the production Uno target produced no size improvement and added irrelevant C-compiler warnings. It was reverted and is not part of the accepted production optimization.

### Narrow compact hexadecimal parser

Commit:

```text
255f83417a5486117fe48a30a148e86e93c504e4
opt(uno): narrow compact hex parser
```

`Arduino/CM_CompactStrtoul.cpp` was narrowed to the actual Uno use: hexadecimal parsing used by CRC16 fields. Unsupported generic `strtoul` behavior such as whitespace/sign/`0x` handling was removed because the existing callers already validate a canonical four-character CRC field before invoking it.

Exact verification:

```text
Arduino Uno Build #256
run 33727723569
completed/success

CMP Protocol Tests #4820
run 33727723542
completed/success
```

Measured resources:

```text
RAM:   1227 / 2048 bytes; 821 bytes free
Flash: 31126 / 32256 bytes; 1130 bytes free
```

Improvement from #255: 80 bytes Flash.

### Exact four-hex CRC parser specialization

Commit:

```text
f812a8cef84d98bccb890a216ee7699aa3ce28f5
opt(uno): specialize CRC hex parser
```

The compact parser now performs exactly the four hexadecimal digits required by both Uno `parseHex16` callers and accumulates the value as `uint16_t` instead of a generic 32-bit `unsigned long` operation path.

Existing callers still enforce:

- non-null input;
- exactly four characters;
- base 16;
- complete token consumption;
- CRC16 value semantics.

Exact verification:

```text
Arduino Uno Build #257
run 33727817804
completed/success

CMP Protocol Tests #4821
run 33727817692
completed/success
```

Measured resources:

```text
RAM:   1227 / 2048 bytes; 821 bytes free
Flash: 31114 / 32256 bytes; 1142 bytes free
```

Improvement from #256: 12 bytes Flash.

## Current accepted Uno checkpoint

Current code checkpoint at the end of this optimization block:

```text
f812a8cef84d98bccb890a216ee7699aa3ce28f5
```

Current measured headroom:

```text
Flash free: 1142 bytes
RAM free:    821 bytes
```

Compared with the recent 31758-byte Flash state that left only 498 bytes free, the accepted optimization chain reduces Flash use by 644 bytes while leaving runtime behavior unchanged.

## Safety invariants unchanged

This block does not change:

- physical START requirements;
- reboot/no-auto-resume behavior;
- SSR ownership;
- UART/CMP frame format;
- Hall calibration ownership or safety flow;
- RUN_STARTED / RUN_COMPLETED evidence semantics;
- manual exact-spool wire write-off requirements.

In particular:

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web do not directly control SSR;
- RUN_COMPLETED alone does not write off wire;
- wire write-off stays manual and tied to exact `spool_id`, `source_session_id` and `source_run_id`.

## Next step

Do not continue speculative parser/compiler micro-optimization merely to gain a few bytes.

The current 1142-byte Flash headroom is materially safer than the pre-optimization state. Further Uno optimization should be driven by a measured need or a clearly identified duplicated/heavy symbol with preserved behavior.

Prefer next:

1. preserve this resource checkpoint;
2. continue functional project work or physical/runtime verification;
3. if more Uno space becomes necessary, profile/select a specific high-cost implementation block rather than broad compiler experiments.

Older PROJECT_HANDOFF files may retain historical branch names as records of when those checkpoints were created. For current branch policy, this checkpoint is authoritative.
