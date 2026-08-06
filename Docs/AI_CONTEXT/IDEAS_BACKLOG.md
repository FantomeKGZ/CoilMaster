# CoilMaster OS — Ideas and Backlog

Этот файл содержит только будущие идеи, предложения и незавершённые задачи. Реализованные пункты переносятся в `IMPLEMENTATION_LOG.md`, но остаются здесь с пометкой `DONE` и ссылкой на реализацию.

Priority levels:

- `P0` — blocks current development;
- `P1` — required for first working system;
- `P2` — important improvement;
- `P3` — future enhancement;
- `IDEA` — requires analysis before scheduling.

## CMP Core

### P0 — Unify CMP public naming

Status: `IN PROGRESS`

Tasks:

- migrate `CMP_Flags.h` to namespace `CMP`;
- choose final type model: bitmask enum plus helper functions;
- migrate `CMP_Command.h` to `CMP::Command`;
- migrate `CMP_Header.h` to `CMP::Header`;
- update all dependent code;
- preserve binary packet layout.

### P0 — Implement and test CMP CRC

Status: `PLANNED`

Requirements:

- CRC16-CCITT;
- polynomial `0x1021`;
- initial value `0xFFFF`;
- incremental `update()` support;
- whole-buffer `calculate()` support;
- deterministic test vectors;
- no dynamic memory;
- host-side test where possible.

### P0 — Implement CMP Packet

Status: `PLANNED`

Requirements:

- fixed header;
- static payload array up to 128 bytes;
- CRC field;
- clear/reset method;
- payload length validation;
- no packed assumptions for internal processing unless explicitly serialized.

### P0 — Implement CMP Buffer

Status: `PLANNED`

Requirements:

- ring buffer or equivalent fixed-size storage;
- random `peek(index)`;
- batch peek;
- discard only after validation;
- overflow reporting;
- parser-compatible synchronization search.

### P0 — Implement CMP Parser

Status: `PLANNED`

States:

- `WAIT_START`;
- `READ_HEADER`;
- `READ_PAYLOAD`;
- `READ_CRC`;
- `VERIFY`;
- `READY`.

Important behaviour:

- never read UART directly;
- never discard a candidate packet before full validation;
- recover synchronization by locating the next start word;
- reject incompatible versions;
- validate payload length before reading payload.

### P1 — Implement CMP Protocol and transport adapter

Status: `PLANNED`

Separate CMP protocol logic from UART hardware.

Possible interfaces:

- byte source/sink abstraction;
- Arduino `Stream` adapter;
- UNO `SoftwareSerial` or hardware UART adapter if required;
- ESP32 `HardwareSerial` adapter.

### P1 — Implement CMP Dispatcher

Status: `PLANNED`

Requirements:

- fixed handler registration without heap allocation;
- command lookup;
- request/response handling;
- ACK/NACK rules;
- unknown-command result;
- no application logic inside protocol layer.

## Build and quality

### P0 — Add dual-platform compilation checks

Status: `PLANNED`

Compile shared protocol code for:

- Arduino UNO / AVR;
- ESP32.

Consider PlatformIO or Arduino CLI for repeatable builds.

### P1 — Add host-side unit tests

Status: `PLANNED`

Focus first on:

- CRC;
- flags;
- header serialization;
- packet validation;
- ring buffer;
- parser resynchronization.

### P1 — Clean legacy duplicate architecture

Status: `PLANNED`

Review `Core/CM_System/` and determine:

- archive;
- migrate useful code;
- remove after dependency confirmation.

Do not delete before confirming no active include or build depends on it.

### P2 — Clean escaped Markdown documents

Status: `PLANNED`

Some documents contain escaped headings and excessive blank lines. Clean formatting without changing approved content.

## Arduino UNO firmware

### P1 — Complete HAL layer

Status: `PLANNED`

Modules:

- Hall sensor;
- LCD1602;
- keypad;
- SSR;
- buzzer;
- UART transport.

### P1 — Winding state machine

Status: `PLANNED`

Possible states:

- boot;
- idle;
- ready;
- winding;
- paused;
- completed;
- warning;
- fault;
- emergency stop.

Requires separate approved specification before implementation.

### P1 — Turn counting reliability

Status: `PLANNED`

Topics:

- interrupt-driven Hall event capture;
- debounce/noise filtering;
- atomic counter access;
- missed-pulse diagnostics;
- calibration mode;
- persistent progress recovery.

### P2 — Safe SSR control

Status: `PLANNED`

Requirements:

- fail-safe default OFF;
- startup lockout;
- watchdog interaction;
- command timeout;
- emergency stop path independent of ESP32 services.

## ESP32 firmware

### P1 — Service boot manager

Status: `PLANNED`

Initialize in controlled order:

1. logging;
2. configuration;
3. RTC;
4. microSD;
5. CMP link;
6. Wi-Fi;
7. web services;
8. backup and maintenance services.

### P1 — microSD storage layout

Status: `PLANNED`

Define directories for:

- configuration;
- motor database;
- logs;
- backups;
- handbook;
- web assets;
- update packages.

### P1 — Motor database

Status: `PLANNED`

Store motor winding data, repair history and calculation results.

Need schema, versioning and backup strategy before implementation.

### P1 — Web interface

Status: `PLANNED`

Functions:

- system status;
- winding progress;
- motor database;
- logs;
- settings;
- backup/restore;
- diagnostics.

Web interface must not directly manipulate hardware; it sends approved commands through application services and CMP.

### P2 — OTA update system

Status: `PLANNED`

Needs:

- signed or verified package strategy;
- rollback;
- power-loss protection;
- version compatibility rules;
- separate ESP32 and UNO firmware update procedures.

## Calculation and business logic ideas

### IDEA — Motor winding recalculation web generator

Status: `REQUIRES SPECIFICATION`

Purpose:

Recalculate an electric motor winding from aluminium wire to copper wire while considering materials available in stock.

Potential inputs:

- original conductor material;
- original wire diameter or section;
- number of parallel wires;
- turns;
- slot dimensions;
- winding connection;
- voltage, current and power;
- motor dimensions;
- available copper diameters and quantities in stock;
- permissible fill factor;
- thermal and current-density constraints.

Potential outputs:

- equivalent copper cross-section;
- recommended stock wire combination;
- adjusted turns if needed;
- slot fill estimate;
- resistance comparison;
- estimated losses and current density;
- warnings when no safe stock combination exists;
- printable winding card;
- save result to motor database.

This feature requires validated engineering formulas and domain rules before being used for real repair decisions.

### IDEA — Warehouse wire inventory

Status: `REQUIRES SPECIFICATION`

Track:

- material;
- diameter;
- insulation type;
- estimated remaining mass/length;
- supplier;
- batch;
- location;
- reservations for active jobs.

Integrate with the winding recalculation generator.

### IDEA — Repair job workflow

Status: `REQUIRES SPECIFICATION`

Stages:

- intake;
- measurements;
- original winding record;
- recalculation;
- stock allocation;
- winding;
- testing;
- report and archive.

## Diagnostics and resilience

### P2 — Persistent event journal

Status: `PLANNED`

Record:

- boots and resets;
- faults;
- winding start/stop/pause;
- operator actions;
- protocol errors;
- storage errors;
- network events;
- software versions.

### P2 — Watchdog strategy

Status: `PLANNED`

Define separately for UNO and ESP32. A service failure on ESP32 must not leave UNO hardware in an unsafe state.

### P2 — Power-loss recovery

Status: `PLANNED`

Persist enough state to safely resume or recover an interrupted winding operation after operator confirmation.

## Documentation process

### P1 — Module status table

Status: `PLANNED`

Add a machine-readable or Markdown table with:

- module;
- owner controller;
- version;
- status;
- dependencies;
- tests;
- last commit;
- next action.

### P2 — Automatic context update helper

Status: `IDEA`

Create a script that gathers:

- latest commit;
- changed files;
- open tasks;
- build status;
- test status;

and prepares updates for `PROJECT_STATE.md`, `IMPLEMENTATION_LOG.md` and `SESSION_LOG.md`.
