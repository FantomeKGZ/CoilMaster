# CoilMaster — Hall telemetry GREEN and legacy CAL_RESULT audit

Date: **2026-08-25**  
Repository: `FantomeKGZ/CoilMaster`  
Source of truth: **`cmp-protocol-v1` only**.

## Verified CI — Hall telemetry hardening

The Hall telemetry semantic-validation block is now fully verified:

```text
32762099225  ESP32 Build         checkout a18158da211ab3e36fe043f80e278c252d184312  SUCCESS
32762099263  CMP Protocol Tests  checkout a18158da211ab3e36fe043f80e278c252d184312  SUCCESS
32762184024  CMP Protocol Tests  checkout 94ec3d6749debc09d5406eb49612125f060c4611  SUCCESS
32762259476  CMP Protocol Tests  checkout 1c7cf1aa4943285f26cd79683ccfdb15ac9e8ca1  SUCCESS
```

The descendant host-test run passed all listed Hall, release-safety, JOB lifecycle, backup/restore, material/writeoff and NDJSON contract audits.

Hardware is **not** thereby declared GREEN.

## Verified Hall telemetry receive semantics

ESP32 `HardwareControlClient::processTelemetryState()` now rejects a CRC-valid `HALL_STATE` frame before mirroring it unless:

```text
rawAdc              0..1023
windowMin           0..1023
windowMax           0..1023
windowMin <= rawAdc <= windowMax
threshold           1..1023
hysteresis          1..512 and < threshold
releaseDebounceMs   1..1000
sampleCount         > 0
releaseBoundary     exactly matches threshold/hysteresis/direction
```

Direction, magnet flag and re-arm state also remain strict-enum fields.

## Legacy CAL_RESULT receive audit

The active Uno completion path remains compact:

```text
CMP1|CAL_DONE|measurement_id|C|CRC
```

Uno no longer emits legacy `CAL_RESULT` as the active completion format.

The ESP32 legacy `CAL_RESULT` parser remains intentionally present for backward receive compatibility. Repository search found no current `CAL_RESULT|VALID` emitter or active wire path to use as an authoritative compatibility contract. The current Uno legacy formatter retained in history emits only the invalid placeholder form with zeroed statistics plus measurement id.

Therefore this audit makes **no compatibility-breaking parser change**. Do not delete or narrow the legacy receive path until an explicit compatibility-removal decision or an authoritative historical valid-frame contract is available.

## Current software status

Verified GREEN blocks now include:

- compact Uno `CAL_DONE` completion;
- exact 77-byte Hall response formatter bound;
- ESP32 `CAL_APPLIED` profile range validation;
- ESP32 `CFG_STATE` profile range validation;
- ESP32 `HALL_STATE` telemetry semantic validation.

Continue targeted software review only. Do not request intermediate physical hardware tests. Final physical acceptance remains one consolidated E2E gate after software optimization is complete.
