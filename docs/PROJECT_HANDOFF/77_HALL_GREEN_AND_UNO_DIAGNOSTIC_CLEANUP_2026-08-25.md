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

## HALL_STATE telemetry validation-order cleanup — software CI GREEN

A narrow review of `HardwareControlClient::processTelemetryState()` confirmed a low-risk correctness issue: for Rising telemetry the expected release boundary was calculated before threshold/hysteresis validity was checked. Invalid profiles were rejected later, so there was no acceptance bypass, but an invalid `hysteresis >= threshold` frame could make unsigned subtraction wrap before that rejection.

Implementation:

```text
8586c4a4dc7286ea4598dfe81225e34212a9c849
fix(hall): validate telemetry profile before boundary math
```

Dedicated regression:

```text
ac1b36cd5259d25632953168358b68a45f8132ee
test(hall): require telemetry validation before boundary math
```

CI integration:

```text
202949005b34b1a4db6052fb99b8fb4ba7223bf5
ci(hall): run telemetry validation order audit
```

Verified runs:

```text
32799148672  CMP Protocol Tests @ 8586c4a4... SUCCESS
32799148673  ESP32 Build @ 8586c4a4... SUCCESS
32799235059  CMP Protocol Tests @ ac1b36cd... SUCCESS
32799255011  CMP Protocol Tests @ 20294900... SUCCESS
32799278211  CMP Protocol Tests @ a9ed7ef6... SUCCESS
```

The descendant workflow explicitly executed `Audit Hall telemetry validation order` and it passed. The ESP32 parser now validates ADC/window/profile/debounce/sample semantics before computing the release boundary. CMP1 wire format and Uno behavior are unchanged.

## Uno hardware-control TX frame bound — current software-only cleanup

Review of the old SRAM-recovery backlog found one remaining bounded peak-stack cost: `HardwareControlProtocol::MaxFrameLength` was still `176U`, even though Hall calibration TX already uses a separate exact 77-byte bound.

The hardware-control formatter accepts `HallTelemetrySnapshot` fields directly, so the safe bound must be based on the field types, not only normal ADC/settings semantics. Exact worst case is:

```text
CMP1|HALL_STATE|65535|65535|65535|65535|65535|65535|65535|FALLING|1|RELEASE_DEBOUNCE|65535|4294967295|C|FFFF\n
```

This is 109 wire bytes; one trailing NUL requires a 110-byte formatter buffer.

Implementation history:

```text
c2a785ab...  initial bounded TX change; superseded before verification
ce760d45099dfa65de735df0b0a6c4eb1489c18b
fix(uno): correct Hall telemetry TX bound
```

Current authoritative bound:

```text
HardwareControlProtocol::MaxFrameLength = 110U
```

This reduces the three hardware-control TX stack buffers from 176 to 110 bytes, saving 66 bytes of peak local stack when those formatters execute. It does not change static `.bss` SRAM, the persistent RX `MaxReplyLength=112`, queue capacity, protocol fields, physical START, SSR, EEPROM or calibration authority.

Regression:

```text
d08816b29ef663b87ce9b61959e8b9332aa9eeba
test(uno): protect hardware TX frame bound
```

The regression independently checks the 109-byte worst-case wire fixture, requires the 110-byte compile-time bound and protects use of `HardwareControlProtocol::MaxFrameLength` in the UART TX stack buffers.

## Verification state

- Hall rejected-ARM / APPLY-ABORT orchestration: software CI GREEN.
- Uno temporary diagnostic cleanup: software CI GREEN.
- HALL_STATE validation-order batch: software CI GREEN.
- Uno hardware-control TX bound `ce760d45...` + regression `d08816b2...`: **fresh Arduino Uno Build / CMP Protocol Tests pending**.
- Final two-board physical acceptance remains deferred until software optimization is complete.

## Next software step

Verify the Uno TX-bound batch with fresh Arduino Uno Build and CMP Protocol Tests. If GREEN, continue narrow software-only review; do not change legacy `CAL_RESULT|VALID` semantics without authoritative contract evidence and do not request intermediate hardware testing.
