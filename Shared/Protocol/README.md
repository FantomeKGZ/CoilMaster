# Shared/Protocol — binary host-test CMP

This directory contains the earlier binary CMP v1 implementation used by
`Tests/Protocol`. It is retained as host-test/compatibility code only.

It is **not** the production ESP32 <-> Arduino wire protocol.

Production UART uses text `CMP1|...` frames implemented by:

```text
Arduino/CM_UartEventTransport.*
firmware/esp32/src/CM_UartEventReceiver.*
Shared/CMP1Text/CM_Cmp1Crc.h
```

Do not replace the production text transport with this binary protocol without
an explicit cross-board migration and Arduino SRAM/compatibility review.

## Binary CMP v1 packet format

The host-test protocol uses little-endian fields:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 2 | start word `0xAA55` |
| 2 | 1 | protocol major |
| 3 | 1 | protocol minor |
| 4 | 1 | flags |
| 5 | 1 | reserved (`0`) |
| 6 | 2 | command identifier |
| 8 | 2 | packet counter |
| 10 | 2 | payload length |
| 12 | N | payload (`0..128` bytes) |
| 12 + N | 2 | CRC16-CCITT |

Maximum encoded packet size is 142 bytes.

CRC parameters:

```text
polynomial: 0x1021
initial:    0xFFFF
final XOR:  none
test vector "123456789": 0x29B1
```

The parser is fixed-capacity/no-heap and validates version, flags, reserved
byte, payload length and CRC before exposing a packet.

Breaking changes to this binary format require corresponding updates in
`Tests/Protocol`. They do not by themselves change production CMP1 UART.
