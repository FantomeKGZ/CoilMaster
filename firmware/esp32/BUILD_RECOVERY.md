# ESP32 build recovery marker

Date: 2026-08-22

This file is intentionally kept under `firmware/esp32/**` so a documentation-only recovery commit can trigger the existing `ESP32 Build` workflow without changing runtime firmware behavior.

Current candidate being verified by this trigger:

- source of truth: `cmp-protocol-v1`
- lifecycle hardening includes timeout manual-review isolation, stale terminal cancel no-op, immutable repeat-target journal guards, and snapshot/state repeat-target integrity checks
- latest confirmed protocol result before this trigger: `CMP Protocol Tests #2210` GREEN on `ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133`
- this marker changes documentation only; production firmware semantics are unchanged by this trigger commit
- hardware acceptance is not implied by a CI build

The resulting ESP32 workflow run is the authoritative compile check for the current lifecycle-hardening source tree.
