# CMP1 text / UART audit — 2026-08-22

Branch: `cmp-protocol-v1`

## Verification baseline

User explicitly confirmed the current audit batch GREEN at:

```text
b358f308e032bf484ebfc37b17d29334216e8424
Record Core state-machine audit findings
USER CONFIRMED GREEN
```

This confirmation covers the preceding Arduino/Core audit chain, including the LCD sync-column fix, EEPROM metadata write optimization, guarded ordinary return-home transitions, and the JobComplete manual-mode guard.

## Shared/CMP1Text result

Production shared text protocol owner is:

```text
Shared/CMP1Text/CM_Cmp1Crc.h
```

Direct cross-owner inspection was performed against:

```text
Arduino/CM_UartEventTransport.cpp
firmware/esp32/src/CM_UartEventReceiver.cpp
Tests/CMP1Text/test_main.cpp
```

Confirmed:

- both endpoints use the same CRC-16/MODBUS implementation (`initial=0xFFFF`, polynomial `0xA001`);
- event frames calculate CRC over the payload before the final `|CRC` suffix;
- ESP32 JOB frames advertise reply-CRC capability using `|C` and Arduino records that capability only after a valid JOB frame;
- current CRC-protected `JOB_ACK` / `JOB_CANCEL_ACK` format and legacy four-separator replies are both accepted by ESP32;
- a reboot/recovery cancel may therefore receive a legacy Arduino cancel ACK before reply-CRC capability is negotiated without creating a false protocol timeout;
- Arduino accepts both CRC-protected current ACK/NACK and staged legacy ACK/NACK for run-event delivery;
- `RUN_STARTED` requires `completedRuns=0`, while `RUN_COMPLETED` requires non-zero completion evidence on ESP32 parsing;
- local standalone events preserve program type, coil count and exact per-coil turns and are CRC protected.

No confirmed defect was found in the shared CRC implementation or current CMP1 frame agreement in this pass.

## Next audit owner

Continue from ESP32 UART/job state persistence and recovery ownership. Focus on accepted/timed-out/cancelled JOB state, reboot recovery, exact run identity, and whether control-state persistence can diverge from UART reality. Do not weaken the established rules: timeout/lost ACK does not prove Arduino idle; no automatic physical START; RUN_COMPLETED alone never writes off material.
