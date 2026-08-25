# CoilMaster — Uno UART RX and SRAM cleanup checkpoint

Date: **2026-08-25**
Repository: `FantomeKGZ/CoilMaster`
Source of truth: **`cmp-protocol-v1` only**.

## Uno hardware-control TX bound — software CI GREEN

Verified runs:

```text
32799879272  CMP Protocol Tests @ c2a785ab... SUCCESS
32799879295  Arduino Uno Build @ c2a785ab... SUCCESS
32799904601  CMP Protocol Tests @ ce760d45... SUCCESS
32799925667  CMP Protocol Tests @ d08816b2... SUCCESS
32799958618  CMP Protocol Tests @ a5474c5b... SUCCESS
```

Authoritative TX formatter bound:

```text
HardwareControlProtocol::MaxFrameLength = 110U
```

Worst-case `HALL_STATE` is 109 wire bytes plus NUL. This saves 66 bytes of peak local stack versus the old 176-byte bound.

## Uno UART RX bound + counter — software CI GREEN

The longest accepted ESP32 -> Uno JOB line is 106 characters excluding newline: max uint32 job/session ids, `STARTING`, 10 coils, ten `9999` turn targets, `R65535`, capability `C`, and CRC.

Implementation:

```text
be9ad7028f05a7ae37186c4609fbc51f3f0e899b
perf(uno): tighten UART reply buffer

9ff5dbb53f80d7be9f4506e30f579be16405528b
perf(uno): narrow UART reply length counter
```

Current state:

```text
UartEventTransport::MaxReplyLength = 107U
uint8_t m_replyLength;
```

SRAM effect on AVR Uno:

- RX array 112 -> 107: saves 5 bytes persistent SRAM;
- `m_replyLength` `size_t` -> `uint8_t`: saves 1 byte persistent SRAM;
- total RX batch saving: **6 bytes persistent SRAM**.

Regression history:

```text
6005512435c13babe764414c868a343da6022212
test(uno): require exact UART reply bound

bb7882e7db6c8a28d8b26d0d4e63a0425d856dba
test(uno): protect narrow UART reply counter

d977c2f730663fff85afdcb50e942d947f6022be
test(uno): check UART counter in header
```

The first counter regression incorrectly searched the `.cpp` source for a member declared in the header. Firmware was not the failure. Final verification:

```text
32801242190  Arduino Uno Build @ 9ff5dbb5... SUCCESS
32801242173  CMP Protocol Tests @ 9ff5dbb5... SUCCESS
32801535581  CMP Protocol Tests @ d977c2f7... SUCCESS
```

Run `32801535581` explicitly passed `Audit Arduino JOB parser contracts`. Therefore the RX bound/counter batch is **software CI GREEN**.

## Uno buzzer timing state compaction — software CI GREEN

Review of `BuzzerService` found duplicate persistent duration fields:

```text
uint16_t m_onDurationMs;
uint16_t m_offDurationMs;
```

All production patterns use equal ON/OFF durations when an OFF phase exists:

```text
JOB accepted: 80 / 80 ms
coil complete: one 120 ms ON phase, no OFF phase
program complete: 120 / 120 ms
```

Therefore a single phase duration preserves exact current timing semantics.

Implementation:

```text
8db6a321fdb17971d249030dd959bb7d1be0a2c7
perf(uno): compact buzzer phase timing

a224c239dc565c086f82d2deb85e34c888e095c4
perf(uno): reuse buzzer phase duration
```

Current member:

```text
uint16_t m_phaseDurationMs;
```

This removes one `uint16_t` from the persistent buzzer object, saving **2 bytes of static SRAM**. It does not change keypad handling, physical START, SSR, Hall ownership, UART protocol, state-machine authority, or EEPROM behavior.

Regression:

```text
7a03511fa8a7cb3efa0c6322dea94727e6cf4126
test(uno): protect compact buzzer timing
```

Verified operator-supplied runs:

```text
32801765239  CMP Protocol Tests @ 8db6a321... SUCCESS
32801785488  Arduino Uno Build @ a224c239... SUCCESS
32801785553  CMP Protocol Tests @ a224c239... SUCCESS
32801820990  CMP Protocol Tests @ 7a03511f... SUCCESS
32801850029  CMP Protocol Tests @ daad5413... SUCCESS
```

Therefore the buzzer compaction batch is **software CI GREEN**.

## Current measured Uno resource state

The authoritative Arduino Uno Build run `32801785488` at implementation SHA `a224c239...` reports:

```text
RAM:   1205 / 2048 bytes = 58.8%
Flash: 31460 / 32256 bytes = 97.5%
```

Remaining static SRAM headroom is **843 bytes**. Remaining flash headroom is only **796 bytes**.

This changes the optimization priority: Uno SRAM is no longer the immediate resource bottleneck. Do not continue one- or two-byte SRAM micro-optimizations if they add code or complexity. Flash remains the tight resource and must be protected.

## Review decisions — KEEP

### JOB reply TX stack buffer

`writeJobReply()` still uses local `char frame[88]`. Streaming replacement would need additional explicit frame-length enforcement and likely cost scarce Uno flash for only temporary stack savings. Keep it unless measured stack pressure justifies the trade.

### Per-event local program snapshots

The UART event queue intentionally retains a separate `LocalProgramSnapshot` per queued event to preserve exact local winding provenance across delayed acknowledgements. Keep this design.

### Keypad map/pin arrays

The proven `Keypad` library expects RAM-backed map/pin data. Moving them to PROGMEM would require input-path changes and risks repeating the prior keypad regression. Keep unchanged.

### Small persistent bool/enum packing

Several Uno services still contain individual boolean or small enum fields that could theoretically be packed. With current RAM at 58.8% and flash at 97.5%, do **not** spend flash or readability to save isolated bytes unless a measured runtime SRAM problem reappears.

## Verification state

- Uno hardware-control TX bound 110: **software CI GREEN**.
- Uno UART RX bound 107 + byte-sized counter: **software CI GREEN**.
- Uno buzzer timing state compaction: **software CI GREEN**.
- Current measured Uno resource state: **RAM 58.8%, Flash 97.5%**.
- Final two-board physical acceptance remains deferred until software optimization is complete.

## Next software step

End the current Uno SRAM micro-optimization pass. Continue with software-only reliability/review work that does not enlarge the Uno firmware unnecessarily. Prefer ESP32/Web/storage/protocol review or Uno flash-neutral fixes. Keep Keypad, physical START, SSR ownership and Hall authority unchanged.
