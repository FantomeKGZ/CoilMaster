# CoilMaster OS — Architecture Decisions

Этот файл хранит принятые решения. Решение не удаляется: при изменении оно помечается как `SUPERSEDED`, а ниже добавляется новое.

## DEC-001 — Two-controller architecture

Status: `APPROVED`

Arduino UNO is the real-time equipment controller. ESP32 is the service controller.

Reason:

- predictable real-time behaviour on UNO;
- network and storage workload isolated on ESP32;
- failure of a service module must not directly stop low-level control logic.

## DEC-002 — CMP is the only inter-controller protocol

Status: `APPROVED`

All Arduino UNO ↔ ESP32 communication goes through CMP over a transport layer.

Direct access from one controller to hardware owned by the other controller is prohibited.

## DEC-003 — UART transport for current hardware

Status: `APPROVED`

Current transport:

- UART;
- 115200 baud;
- 8N1;
- TXS0108E level conversion;
- common ground.

CMP itself must remain transport-independent so future USB, TCP, Bluetooth, CAN or RS485 transports can be added without changing packet semantics.

## DEC-004 — Hardware access only through HAL

Status: `APPROVED`

Application code must not directly call functions such as `digitalWrite()`, `analogRead()` or hardware-library APIs.

Hardware access must be encapsulated in HAL modules.

Planned UNO HAL modules include:

- `HAL_Hall`
- `HAL_LCD`
- `HAL_Keypad`
- `HAL_SSR`
- `HAL_Buzzer`

## DEC-005 — CMP memory model

Status: `APPROVED`

CMP Core must use:

- static or stack storage;
- fixed maximum payload and packet sizes;
- no `malloc()`;
- no `new`;
- no `delete`;
- no exceptions.

Reason: Arduino UNO RAM limits and deterministic behaviour.

## DEC-006 — CMP packet format v1.0

Status: `APPROVED`

- start word: `0xAA55`;
- little endian;
- 12-byte header;
- payload: 0–128 bytes;
- CRC16-CCITT;
- CRC polynomial: `0x1021`;
- initial value: `0xFFFF`;
- CRC covers header and payload, excluding the CRC field.

## DEC-007 — CMP parser isolation

Status: `APPROVED`

The parser never reads UART directly.

It reads only from `CMP_Buffer`. Transport receives bytes and feeds the buffer. The parser validates complete packets before removing confirmed packet bytes.

On validation failure, synchronization is restored by searching for the next start word.

## DEC-008 — Dependency direction

Status: `APPROVED`

CMP implementation follows the approved dependency matrix. Cyclic dependencies are prohibited. Higher layers may use only public headers of lower layers.

## DEC-009 — Unified CMP namespace

Status: `IN PROGRESS`

New and refactored CMP types should use namespace `CMP`:

- `CMP::Result`
- `CMP::Flag` or `CMP::Flags`
- `CMP::Command`
- `CMP::Header`
- `CMP::Packet`
- `CMP::CRC`
- `CMP::Buffer`
- `CMP::Parser`
- `CMP::Protocol`
- `CMP::Dispatcher`

Legacy global names currently present in some files must be migrated carefully after checking dependencies.

## DEC-010 — Old Core prototype is not the main development line

Status: `APPROVED`

`Core/CM_System/` is an early prototype. Main firmware development uses `Firmware/UNO/`, `Firmware/ESP32/` and shared code under `Shared/`.

The prototype must not be expanded. It may later be archived or removed only after confirming nothing depends on it.

## DEC-011 — Documentation is part of implementation

Status: `APPROVED`

A code change is not complete until relevant documentation, changelog and AI context files are updated.

## DEC-012 — Completion requires verification

Status: `APPROVED`

File existence is not proof of completion. A module is complete only after compilation, testing and explicit status recording.
