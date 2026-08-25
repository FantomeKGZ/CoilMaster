# CoilMaster — Uno UART RX bound checkpoint

Date: **2026-08-25**
Repository: `FantomeKGZ/CoilMaster`
Source of truth: **`cmp-protocol-v1` only**.

## Previous Uno hardware-control TX bound — software CI GREEN

Verified operator-supplied runs:

```text
32799879272  CMP Protocol Tests @ c2a785ab... SUCCESS
32799879295  Arduino Uno Build @ c2a785ab... SUCCESS
32799904601  CMP Protocol Tests @ ce760d45... SUCCESS
32799925667  CMP Protocol Tests @ d08816b2... SUCCESS
32799958618  CMP Protocol Tests @ a5474c5b... SUCCESS
```

The authoritative hardware-control TX formatter bound remains:

```text
HardwareControlProtocol::MaxFrameLength = 110U
```

This is exact for the 109-byte type-safe worst-case `HALL_STATE` wire frame plus NUL and saves 66 bytes of peak local stack versus the old 176-byte bound.

## Uno UART persistent RX buffer tightening — current batch

The remaining persistent UART receive buffer was reviewed against the active ESP32 -> Uno contracts.

The longest accepted incoming line is the full remote JOB frame with:

- max uint32 `job_id`;
- max uint32 `session_id`;
- `STARTING` winding type;
- 10 coils;
- ten `9999` turn targets;
- max repeat target `R65535`;
- capability `C`;
- four-digit CRC.

Exact worst-case line, excluding the terminating newline, is 106 characters. `pollReplies()` needs one additional byte for the in-place NUL terminator.

Implementation:

```text
be9ad7028f05a7ae37186c4609fbc51f3f0e899b
perf(uno): tighten UART reply buffer
```

Current bound:

```text
UartEventTransport::MaxReplyLength = 107U
```

This reduces the persistent `m_reply` array from 112 to 107 bytes, saving 5 bytes of static SRAM without changing the wire protocol, parsing rules, queue capacity, retry semantics, physical START, SSR, EEPROM or Hall authority.

Regression:

```text
6005512435c13babe764414c868a343da6022212
test(uno): require exact UART reply bound
```

The existing Arduino JOB parser contract now requires `MaxReplyLength` to equal the exact worst-case JOB fixture length plus one NUL byte, rather than only enforcing a loose upper bound.

## Uno UART reply length counter narrowing

A follow-up review confirmed that `m_replyLength` can never exceed 106 while `MaxReplyLength` is 107. On AVR, `size_t` is wider than required here, so the counter is now a byte-sized field.

Implementation:

```text
9ff5dbb53f80d7be9f4506e30f579be16405528b
perf(uno): narrow UART reply length counter
```

Current member:

```text
uint8_t m_replyLength;
```

This saves one additional byte of static SRAM on Arduino Uno. Together with the 112 -> 107 RX buffer tightening, this batch saves 6 bytes of persistent SRAM.

Regression:

```text
bb7882e7db6c8a28d8b26d0d4e63a0425d856dba
test(uno): protect narrow UART reply counter
```

The JOB parser contract now protects both the exact 107-byte receive buffer and the byte-sized receive-length counter.

## Review decisions — KEEP

### JOB reply TX stack buffer

`writeJobReply()` still uses a local `char frame[88]`. Active detail tokens are short, but replacing this with streaming would require additional explicit frame-length enforcement to preserve the current fail-closed bound and would likely cost Uno flash. With Uno flash already constrained, this candidate is **KEEP for now**; do not trade scarce flash for a small/temporary stack reduction without measured need.

### Per-event local program snapshots

The four-entry UART event queue intentionally retains a separate `LocalProgramSnapshot` for each queued event. This costs SRAM, but it preserves the exact local winding program when acknowledgements are delayed. Do not merge it into one shared snapshot without a new provenance design and proof that queued events from different local programs cannot coexist.

### Keypad map/pin arrays

The proven Arduino `Keypad` library expects RAM-backed map/pin data. Moving these arrays to PROGMEM would require replacing or adapting the input implementation, which previously caused keypad regressions. Keep the proven Keypad owner unchanged.

## Verification state

- Uno hardware-control TX bound 110: **software CI GREEN**.
- Uno UART RX bound 107 (`be9ad702...` + `60055124...`): fresh Arduino Uno Build / CMP Protocol Tests pending.
- Uno byte-sized RX counter (`9ff5dbb5...` + `bb7882e7...`): fresh Arduino Uno Build / CMP Protocol Tests pending.
- Final two-board physical acceptance remains deferred until software optimization is complete.

## Next software step

After fresh CI for the RX-bound/counter batch, continue only narrow evidence-backed Uno memory/reliability review. Avoid broad rewrites and avoid adding flash-heavy helpers solely to remove small temporary stack buffers.
