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

That documentation commit itself is fresh-CI-pending until a descendant host-tests run is checked.

## Uno temporary diagnostic profile cleanup

The reset-loop investigation is already closed and production Uno no longer needs the temporary PlatformIO environment `uno_diagnostic`.

Current cleanup commit:

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

No Arduino runtime, physical START, SSR, EEPROM, Hall counting, UART protocol or write-off logic changed in this cleanup.

## Verification state

- Hall rejected-ARM Web-state block: **software CI GREEN**.
- Hall APPLY/ABORT follow-up: **review-only; no code change required**.
- `c0781c66...` diagnostic-env cleanup: **fresh Arduino Uno Build / CMP tests pending**.
- Final two-board physical acceptance remains deferred until software optimization is complete.

## Next software step

Verify descendant CI for `c0781c66...`. If GREEN, continue software-only review from the active queue without reintroducing broad cleanup work or intermediate hardware testing.
