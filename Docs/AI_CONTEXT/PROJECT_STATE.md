# CoilMaster OS — Current Project State

Last updated: 2026-08-06
Branch: `main`
Repository: `FantomeKGZ/CoilMaster`
Release: `0.1.0`
Current build/package: `002A / CMP Core`
Overall status: `IN PROGRESS`

## Project purpose

CoilMaster OS is a modular software platform for an electric motor winding machine.

The system uses two independent controllers:

- Arduino UNO — real-time equipment controller.
- ESP32 — service, network, storage and web controller.

Communication between controllers is allowed only through CMP — CoilMaster Protocol.

## Hardware status

Completed and verified before current software stage:

- individual module tests;
- Arduino UNO and ESP32 communication test;
- UART communication at 115200 baud;
- TXS0108E logic-level converter test;
- ESP32 modules including DS3231 and microSD were previously checked.

Do not return to hardware verification unless a new software or integration failure requires it.

## Controller responsibilities

### Arduino UNO

- Hall sensor and turn counting;
- winding real-time control;
- LCD1602;
- keypad;
- SSR;
- buzzer;
- execution of winding commands;
- CMP communication with ESP32.

### ESP32

- Wi-Fi;
- HTTP/Web server;
- FTP;
- microSD;
- RTC DS3231;
- motor database;
- handbook;
- logging;
- backup;
- future OTA.

ESP32 must not directly control winding hardware.

## Main architecture

The intended project structure is based on:

- `Firmware/UNO/`
- `Firmware/ESP32/`
- `Shared/`
- `Docs/`
- `Web/`
- `SD_Image/`
- `Tests/`
- `Tools/`
- `Releases/`

The older `Core/CM_System/` directory is an early prototype. It is not the main implementation path and must not be expanded without a migration decision.

## CMP protocol constants

Current protocol specification:

- version: 1.0;
- transport: UART, 115200 baud, 8N1;
- start word: `0xAA55`;
- byte order: little endian;
- header size: 12 bytes;
- payload size: 0–128 bytes;
- CRC size: 2 bytes;
- maximum packet size: 142 bytes;
- CRC: CRC16-CCITT;
- polynomial: `0x1021`;
- initial CRC value: `0xFFFF`.

Header fields:

1. StartWord — 2 bytes
2. VersionMajor — 1 byte
3. VersionMinor — 1 byte
4. Flags — 1 byte
5. Reserved — 1 byte
6. Command — 2 bytes
7. Counter — 2 bytes
8. PayloadLength — 2 bytes

## CMP dependency order

Implementation must follow this order:

### Level 0

- `CMP_Defines`
- `CMP_Result`
- `CMP_Flags`
- `CMP_Command`

### Level 1

- `CMP_Header`
- `CMP_Packet`
- `CMP_CRC`

### Level 2

- `CMP_Buffer`

### Level 3

- `CMP_Parser`

### Level 4

- `CMP_Protocol`

### Level 5

- `CMP_Dispatcher`

No cyclic dependencies are allowed.

## Current implementation status

### Implemented or started

- `Shared/Protocol/CMP_Defines.h`
  - migrated from macros to `constexpr`;
  - uses namespace `CMP`;
  - contains packet, buffer, UART and CRC constants;
  - includes compile-time checks.

- `Shared/Protocol/CMP_Result.h`
  - migrated to `CMP::Result`;
  - uses explicit stable `uint8_t` values;
  - helper functions: `succeeded()`, `failed()`, `isPending()`;
  - no exceptions and no dynamic memory.

- `Shared/Protocol/CMP_Flags.h`
  - exists, but still uses the older global naming style;
  - requires review and migration to the unified namespace/API.

- `Shared/Protocol/CMP_Command.h`
  - exists with command groups;
  - still uses the older global naming style;
  - requires review and migration to the unified namespace/API.

- `Shared/Protocol/CMP_Header.h`
  - exists as a packed 12-byte structure;
  - requires compatibility review after Flags and Command migration.

- CMP specification, API document and dependency matrix exist.

- UNO Core scaffolding exists under `Firmware/UNO/Core/`:
  - `CM_Config`
  - `CM_Event`
  - `CM_Logger`
  - `CM_Settings`
  - `CM_System`
  - `CM_Version`
  - `CoilMaster_UNO.ino`

These modules are not yet considered complete merely because files exist.

## Current risks and inconsistencies

- Old global CMP types coexist with the new `namespace CMP` style.
- Early prototype code under `Core/CM_System/` duplicates newer architecture.
- Build and test automation is not yet established.
- Current connector changes have not been confirmed by a full Arduino UNO and ESP32 compilation.
- Some documentation files contain escaped Markdown formatting and may need cleanup later.

## Exact continuation point

Continue CMP Core Level 0 in this order:

1. review and refactor `CMP_Flags.h`;
2. review and refactor `CMP_Command.h`;
3. review `CMP_Header.h` for compatibility with the new public types;
4. implement or verify `CMP_Packet`;
5. implement `CMP_CRC` with platform-independent tests;
6. compile shared code for Arduino UNO and ESP32;
7. update context files and changelog.

Do not start Web, Wi-Fi, database or winding application logic until the shared CMP foundation is stable enough for integration.

## Definition of done for a module

A module is complete only when:

- API matches architecture and specification;
- code compiles for all target platforms where applicable;
- tests pass;
- no dynamic allocation is used in CMP Core;
- documentation is updated;
- `CHANGELOG.md` and AI context files are updated;
- status is explicitly changed to `PASS` or `APPROVED`.
