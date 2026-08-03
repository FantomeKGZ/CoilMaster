# CoilMaster Protocol (CMP) v1.0

## Purpose

CMP is the binary transport protocol used between CoilMaster controllers and supporting devices. The implementation is platform-independent and does not depend on Arduino classes, STL containers, dynamic allocation, or a specific UART driver.

## Byte order

All multi-byte integer fields are encoded in little-endian order.

## Packet layout

| Offset | Size | Field |
|---:|---:|---|
| 0 | 2 | Start word (`0xAA55`) |
| 2 | 1 | Protocol major version |
| 3 | 1 | Protocol minor version |
| 4 | 1 | Flags |
| 5 | 1 | Reserved (`0`) |
| 6 | 2 | Command identifier |
| 8 | 2 | Packet counter |
| 10 | 2 | Payload length |
| 12 | N | Payload (`0..128` bytes) |
| 12 + N | 2 | CRC16-CCITT |

The fixed header size is 12 bytes. The maximum encoded packet size is 142 bytes.

## CRC

CMP uses CRC16-CCITT with:

- polynomial: `0x1021`
- initial value: `0xFFFF`
- no final XOR
- CRC covers the serialized header and payload
- CRC bytes are appended little-endian

The standard test vector `123456789` produces `0x29B1`.

## Flags

| Bit | Meaning |
|---:|---|
| 0 | ACK required |
| 1 | ACK packet |
| 2 | NACK packet |
| 3 | Response packet |
| 4 | Broadcast |
| 5 | Compressed payload |
| 6 | Encrypted payload |
| 7 | Reserved; must be zero |

ACK and NACK flags may not be set together.

## Receive path

1. Bytes are appended to `CMP::Protocol`.
2. `CMP::Parser` searches for the start word.
3. The header is decoded explicitly from bytes.
4. Version, flags, reserved byte, and payload length are validated.
5. The parser waits until the full packet is available.
6. CRC is verified before any complete packet is removed.
7. On success, the packet is copied into `CMP::Packet` and removed from the ring buffer.
8. On malformed data, one byte is discarded and synchronization resumes.

## Public modules

- `CMP::CRC`: CRC16-CCITT calculation.
- `CMP::Buffer`: fixed-capacity ring buffer.
- `CMP::Parser`: stream-to-packet parser.
- `CMP::Protocol`: packet creation, serialization, receiving, and reading.
- `CMP::Dispatcher`: command-to-handler routing.

## Memory policy

CMP v1.0 uses no heap allocation. `CMP::Packet` owns a fixed 128-byte payload buffer so parsed packets remain valid after their source bytes are removed from the receive buffer.

## Compatibility policy

Changing any field offset, field width, byte order, CRC parameters, or numeric command identifier is a breaking protocol change and requires a new protocol major version.
