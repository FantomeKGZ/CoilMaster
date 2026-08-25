# CoilMaster — Uno resource headroom guard

Date: **2026-08-25**
Repository: `FantomeKGZ/CoilMaster`
Source of truth: **`cmp-protocol-v1` only**.

## Current measured Uno state

Verified Arduino Uno Build run `32801785488` at implementation SHA `a224c239...`:

```text
RAM:   1205 / 2048 bytes = 58.8%
Flash: 31460 / 32256 bytes = 97.5%
```

Current headroom:

```text
RAM   843 bytes
Flash 796 bytes
```

SRAM is no longer the immediate Uno bottleneck. Flash is now the tight resource. Stop isolated one- or two-byte SRAM micro-optimizations if they add code or complexity.

## CI resource guard — current batch

Implementation:

```text
ccfe0f2f8af92e6d0337ec76da2e7fcbd15e12b6
ci(uno): protect resource headroom
```

`.github/workflows/arduino-uno-build.yml` now captures the exact PlatformIO build output and checks both RAM and Flash after a successful production `pio run -e uno` build.

Required minimum headroom:

```text
RAM   >= 512 bytes
Flash >= 512 bytes
```

The guard fails closed if PlatformIO size lines cannot be parsed or if either resource falls below the margin. It does not change firmware, protocol, EEPROM, Keypad, physical START, SSR or Hall behavior.

Regression:

```text
fefa82ac25f665c1f1cfbe4180424854eccd9225
test(ci): protect Uno resource margin
```

`Tests/Web/check_ci_trigger_contracts.js` protects:

- exact production Uno build command;
- retained build-size log;
- post-build resource-headroom gate;
- 512-byte minimum margin;
- coverage of both RAM and Flash.

## Verification state

Previous Uno TX/RX/buzzer optimization blocks are software-CI GREEN. This new workflow/test-only resource guard is **not yet declared GREEN** until fresh Arduino Uno Build and CMP Protocol Tests verify `ccfe0f2f...` / `fefa82ac...` or a descendant.

Final physical two-board acceptance remains deferred until software optimization/review is complete.

## Next software step

After the guard is GREEN, continue software-only reliability review away from Uno SRAM micro-optimization. Prefer ESP32/Web/storage/protocol work or Uno fixes that are flash-neutral/flash-reducing. Keep the proven Keypad path, physical START authority, Arduino-only SSR authority and autonomous Hall counting unchanged.
