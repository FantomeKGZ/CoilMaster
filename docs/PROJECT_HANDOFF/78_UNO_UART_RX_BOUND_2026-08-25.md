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

Run `32801535581` explicitly passed `Audit Arduino JOB parser contracts`. Therefore the RX bound/counter batch is now **software CI GREEN**.

## Uno buzzer timing state compaction — current batch

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

The existing Arduino cleanup contract now protects the single duration field, exact 80/120 ms pattern constants, and rejects restoration of `m_onDurationMs` / `m_offDurationMs`.

Fresh Arduino Uno Build and CMP Protocol Tests for the buzzer batch are **pending**. Do not call this batch GREEN until verified.

## Review decisions — KEEP

### JOB reply TX stack buffer

`writeJobReply()` still uses local `char frame[88]`. Streaming replacement would need additional explicit frame-length enforcement and likely cost scarce Uno flash for only temporary stack savings. Keep it unless measured stack pressure justifies the trade.

### Per-event local program snapshots

The UART event queue intentionally retains a separate `LocalProgramSnapshot` per queued event to preserve exact local winding provenance across delayed acknowledgements. Keep this design.

### Keypad map/pin arrays

The proven `Keypad` library expects RAM-backed map/pin data. Moving them to PROGMEM would require input-path changes and risks repeating the prior keypad regression. Keep unchanged.

## Verification state

- Uno hardware-control TX bound 110: **software CI GREEN**.
- Uno UART RX bound 107 + byte-sized counter: **software CI GREEN**.
- Uno buzzer timing state compaction (`8db6a321...` + `a224c239...` + `7a03511f...`): **fresh Arduino Uno Build / CMP Protocol Tests pending**.
- Final two-board physical acceptance remains deferred until software optimization is complete.

## Next software step

Verify the buzzer compaction batch with fresh Arduino Uno Build and CMP Protocol Tests. If GREEN, continue only narrow evidence-backed Uno memory/reliability review; avoid keypad/START/SSR rewrites and avoid flash-heavy changes for tiny stack wins.
