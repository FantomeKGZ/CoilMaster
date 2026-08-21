# ESP32 build recovery marker

Date: 2026-08-21

This file is intentionally kept under `firmware/esp32/**` so a documentation-only recovery commit can trigger the existing `ESP32 Build` workflow without changing runtime firmware behavior.

Current recovery baseline before this trigger:

- source of truth: `cmp-protocol-v1`
- `CMP Protocol Tests #2170`: GREEN on `2c00b2c8d57e4f4dd0806ac29be7f24893ebf2c2`
- ESP32 runtime sources were not changed by the protocol-contract recovery commits after `a141f7d5fcf9216a178ca31dbefc6189638f8e22`
- hardware acceptance is not implied by a CI build

The resulting ESP32 workflow run is the authoritative compile check for the current recovery state.
