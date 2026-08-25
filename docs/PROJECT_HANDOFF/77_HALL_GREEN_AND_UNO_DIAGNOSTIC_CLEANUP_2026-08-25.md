# CoilMaster — Hall GREEN checkpoint and Uno diagnostic cleanup

Date: **2026-08-25**
Repository: `FantomeKGZ/CoilMaster`
Source of truth: **`cmp-protocol-v1` only**.

## Hall Web-state preservation — software CI GREEN

User-supplied Actions runs verified:

```text
32796735687  CMP Protocol Tests
checkout ba7d49fe6b2e48f361fb78890830d98b6b814e71
host-tests SUCCESS
Hall calibration safety contracts SUCCESS

32796735701  ESP32 Build
checkout ba7d49fe6b2e48f361fb78890830d98b6b814e71
build-esp32 SUCCESS

32796795161  CMP Protocol Tests
checkout ad0015cab7fd1840b0b02f818131cc297c9c3099
host-tests SUCCESS
Hall calibration safety contracts SUCCESS

32796823947  CMP Protocol Tests
checkout fef31191667ecc44b8eb57a274241aafd5b7fded
host-tests SUCCESS
```

This closes software-CI verification for rejected-ARM Web-state preservation. Hardware is not thereby declared GREEN.

## APPLY / ABORT orchestration review

Follow-up review found no new code change required:

- rejected APPLY does not create `m_pendingHistoryMeasurementId`;
- rejected ABORT does not set `m_pendingHistoryAbort`;
- `HardwareControlClient::queueRequest()` rejects a new request while an unread `m_hasReply` exists, preventing a stale buffered reply from being attached to a newly accepted calibration history operation;
- the single Hall control lane prevents unrelated Hall requests from interleaving with staged apply/abort reconciliation.

The detailed review is recorded in `76_HALL_STALE_LEGACY_RESULT_GUARD_2026-08-25.md` by commit:

```text
2a12af75b1d8c6853ddc4a60ff2ae1d8544216d2
docs(handoff): close Hall orchestration review
```

Run `32797758790` verified this descendant host-tests checkpoint GREEN.

## Uno temporary diagnostic profile cleanup — software CI GREEN

The reset-loop investigation is closed and production Uno no longer needs the temporary PlatformIO environment `uno_diagnostic`.

Implementation:

```text
c0781c664ff83ded464e8f3ef732990a19532011
cleanup(uno): remove temporary diagnostic env
```

Only the temporary `[env:uno_diagnostic]` block was removed from `platformio.ini`.

Production remains:

```text
default_envs = uno
[env:uno]
SERIAL_RX_BUFFER_SIZE=8
SERIAL_TX_BUFFER_SIZE=8
```

Verification supplied by operator:

```text
32797818090  ESP32 Build
checkout c0781c664ff83ded464e8f3ef732990a19532011
build-esp32 SUCCESS

32797818106  Arduino Uno Build
checkout c0781c664ff83ded464e8f3ef732990a19532011
build-uno SUCCESS
```

The first CMP runs on `c0781c66...` and its documentation descendant failed only because `Tests/Protocol/check_arduino_lcd_contracts.js` still required the retired diagnostic environment. Firmware build and CTest were not the failure.

The stale contract was corrected by:

```text
5514a20da5ee856e0f99886a4f026a5bfad9655b
test(uno): retire diagnostic environment contract
```

Fresh verification:

```text
32798067981  CMP Protocol Tests
checkout 5514a20da5ee856e0f99886a4f026a5bfad9655b
host-tests SUCCESS
Arduino LCD layout contracts SUCCESS
Hall/release/storage audits SUCCESS
```

Therefore the temporary Uno diagnostic profile cleanup is now **software CI GREEN**.

No Arduino runtime, physical START, SSR, EEPROM, Hall counting, UART protocol or write-off logic changed in this cleanup.

## Current software-only review

A narrow review of `HardwareControlClient::processTelemetryState()` confirmed one low-risk correctness cleanup still exists: for Rising telemetry the expected release boundary is currently calculated before threshold/hysteresis validity is checked. An invalid profile such as `hysteresis >= threshold` is rejected later, so this is not an acceptance bypass, but the intermediate unsigned subtraction can wrap before that reject.

Next change should be minimal and ESP32-only:

1. validate ADC window, threshold/hysteresis, debounce and sample count first;
2. calculate `expectedReleaseBoundary` only after the profile is known valid;
3. retain the exact release-boundary equality check;
4. add/adjust the Hall contract regression without changing the wire format.

Final two-board physical acceptance remains deferred until software optimization is complete.
