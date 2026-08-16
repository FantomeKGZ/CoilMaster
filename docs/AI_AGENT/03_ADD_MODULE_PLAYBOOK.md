# CoilMaster — add-module playbook for AI agents

Use this playbook whenever adding a new hardware module, C++ service, API domain, persisted dataset or operator UI feature.

The purpose is to prevent orphan modules that compile but are not correctly owned, initialized, persisted, backed up, exposed or tested.

## 1. Define the module before writing code

Write a short module manifest in the task notes:

```text
Name:
Board/runtime: Arduino | ESP32 | Browser
Owner: Arduino main | ESP32 main | StaticSiteServer | <parent service>
Responsibility:
Inputs/dependencies:
Outputs/public contract:
Persistent paths:
HTTP routes:
UART frames:
Hardware pins/bus:
Initialization failure behavior:
Runtime/tick behavior:
Safety impact:
Tests/builds required:
Hardware regression required:
```

If the owner or failure behavior is unclear, do not integrate the module yet.

## 2. Pick the correct production location

### Arduino realtime/domain behavior

```text
Core/
```

Use when logic should be independent of direct hardware-library calls.

### Arduino hardware adapter

```text
Arduino/CM_<Name>.h
Arduino/CM_<Name>.cpp
```

Wire it through:

```text
firmware/arduino/src/main.cpp
```

Pins belong in:

```text
Arduino/Config/CM_Pins.h
```

### ESP32 production service

```text
firmware/esp32/src/CM_<Name>.h
firmware/esp32/src/CM_<Name>.cpp
```

Most top-level services are owned by:

```text
firmware/esp32/src/main.cpp
```

System/static web helpers may instead be owned by:

```text
CM_StaticSiteServer.h/.cpp
```

### Browser/UI feature

```text
firmware/esp32/web/desktop/
firmware/esp32/web/mobile/
firmware/esp32/web/shared/
```

Substantial operator UI normally requires desktop + mobile parity.

## 3. C++ module design pattern

Prefer explicit dependencies over hidden global state.

Conceptual pattern:

```cpp
class ExampleService
{
public:
    ExampleService(fs::FS& storage, WebServer& server);

    bool begin();
    void update();       // only if periodic work is required
    bool ready() const;

private:
    fs::FS& m_storage;
    WebServer& m_server;
    bool m_ready = false;
};
```

Not every module needs every method. The important part is explicit ownership and lifecycle.

### Constructor

Inject long-lived dependencies such as:

- `fs::FS&`
- `WebServer&`
- domain store/service reference
- clock/network manager reference

Avoid creating a second independent store for the same authoritative domain unless there is a deliberate reason.

### `begin()`

Initialization should:

- validate required dependencies/storage;
- register routes once where applicable;
- validate/recover persisted state according to the domain contract;
- leave the module in a known `ready/not-ready` state;
- fail closed when correctness cannot be proven.

Do not convert initialization failure into fabricated/default production data.

### `update()` / `loop()`

Use only for bounded state-machine work that cannot be event-driven.

Rules:

- no long blocking scan in the hot loop;
- no unbounded network wait;
- no implicit physical action from remote state;
- retries must be bounded and observable;
- persistent transitions need explicit recovery semantics.

## 4. Adding an ESP32 service module

Checklist:

1. Verify `CM_<Name>.h/.cpp` do not already exist.
2. Identify the domain owner.
3. Add the smallest header/API surface.
4. Add implementation with explicit readiness/failure semantics.
5. Fetch current owner file and wire object construction.
6. Call `begin()` at the correct startup phase.
7. Add `update()` to the main loop only if required.
8. Expose status/diagnostics if the operator needs to distinguish ready vs failed.
9. Add tests/contract audits.
10. Update `01_PROJECT_MAP.md` and `02_CHANGE_ROUTER.md` if this becomes a new maintenance domain.

Because `platformio.ini` includes `firmware/esp32/src/*.cpp`, a new `.cpp` in that directory enters the ESP32 build automatically. Integration still requires an owner; compilation alone is not integration.

## 5. Adding an ESP32 HTTP/API module

Recommended split:

```text
CM_<Domain>Store / Service        authoritative domain behavior
CM_<Domain>Web                    HTTP parsing/status/serialization
```

HTTP layer responsibilities:

- strict method/path/input parsing;
- bounded field lengths and numeric ranges;
- explicit error/status semantics;
- no browser-trusted authoritative calculations;
- call domain API only after validation;
- return authoritative persisted/server results.

Domain layer responsibilities:

- enforce business invariants even if HTTP validation is bypassed;
- own persistence/transaction rules;
- return enough information for the web layer to report the actual result.

### Registration

Determine whether the web object belongs in:

```text
main.cpp
```

or:

```text
CM_StaticSiteServer
```

Do not create a temporary/local `*Web` object that dies after route registration if handlers capture object state.

## 6. Adding a new persistent dataset or file

Before writing the first byte, define:

```text
Path: /data/<domain>/<name>
Record/schema version:
Write model: append | atomic replace | pending/commit | immutable snapshot
Identity key:
Cross references:
Recovery rule:
Corruption behavior:
Backward compatibility:
Backup/restore inclusion:
```

### Mandatory integration checklist

A production persisted domain is incomplete until applicable items are covered:

- writer;
- authoritative reader;
- strict syntax/schema validation;
- duplicate/identity rules;
- cross-reference validation;
- reboot/interrupted-write recovery;
- integrity audit;
- backup manifest/export;
- remote backup inspection/staging;
- restore plan;
- rollback snapshot/apply;
- UI/API access;
- tests.

Do not add a hidden `/data` file outside backup/integrity awareness.

### NDJSON guidance

Current project strategy intentionally remains file/NDJSON based. Do not introduce a database, arbitrary rotation threshold or optimistic persistent cache solely because a file may grow. Use measured device data before architectural migration.

## 7. Adding a new hardware peripheral to ESP32

First inspect the current hardware reference:

```text
docs/PROJECT_HANDOFF/02_ARCHITECTURE_AND_HARDWARE.md
```

Reserved ESP32 pins currently include:

```text
GPIO16/17        UART2 to Arduino
GPIO5/18/19/23  microSD SPI
GPIO21/22        RTC I2C
```

Document before integration:

- exact module model;
- supply voltage and current requirement;
- GPIO logic level;
- bus (I2C/SPI/UART/digital/analog);
- selected pins/address/CS;
- shared-bus compatibility;
- startup state;
- behavior if module is missing;
- whether absence blocks production or only disables an optional feature.

### Bus rule

Prefer sharing an existing bus correctly over inventing conflicting pins, but only after checking addresses, chip-select behavior and electrical levels.

### Power rule

Do not infer safe power voltage from connector labels alone. Preserve common signal ground and avoid accidental back-powering through USB/GPIO.

## 8. Adding a new Arduino peripheral

Arduino Uno is resource constrained and owns realtime safety.

Before adding:

- inspect `Arduino/Config/CM_Pins.h`;
- inspect `platformio.ini` build flags/buffer sizes;
- check pin conflict with START/SSR/Hall/UART/keypad;
- estimate SRAM/flash impact;
- avoid large dynamic strings and unbounded buffers;
- keep slow I/O out of timing-sensitive state transitions;
- ensure the peripheral cannot directly bypass the machine state model.

Wire hardware abstraction in `Arduino/`, domain behavior in `Core/`, and composition in `firmware/arduino/src/main.cpp`.

## 9. Adding a new operator UI feature

Checklist:

1. Define server contract first.
2. Implement/verify API.
3. Add desktop UI.
4. Add mobile UI.
5. Put shared browser logic in `web/shared/` when reasonable.
6. Update navigation in both variants.
7. Add Web audits for page existence, route use and safety-critical labels/contracts.
8. Show server failures visibly.
9. Do not recalculate historical persisted cost from current browser inputs.
10. Do not represent queued/accepted remote state as physical machine motion.

## 10. Adding a protocol capability

Treat this as a cross-board migration, not a local module.

Required work areas:

```text
Arduino/CM_UartEventTransport.*
firmware/esp32/src/CM_UartEventReceiver.*
Shared/CMP1Text/CM_Cmp1Crc.h if checksum behavior changes
Tests/Protocol/
Tests/CMP1Text/
protocol documentation
```

Define:

- new frame grammar;
- exact field bounds;
- capability negotiation/versioning if needed;
- old Arduino behavior;
- old ESP32 behavior;
- invalid/unknown frame behavior;
- timeout/retry behavior;
- upgrade order.

Remote acceptance must never become remote physical START.

## 11. Adding diagnostics

Prefer diagnostics to be read-only and observational.

A diagnostic endpoint should explicitly say whether data is available rather than changing production state to make it available.

For storage, network, reset/brownout or similar status:

- keep read paths separate from repair actions;
- do not silently clear evidence;
- do not auto-delete production data;
- expose enough state to distinguish unavailable vs zero/empty.

## 12. Error and recovery design

Every new module must define what happens on:

- missing microSD;
- malformed persisted row;
- interrupted write;
- reboot during operation;
- duplicate request/event;
- stale identity/reference;
- dependency not ready;
- timeout;
- partial remote transfer if networked.

Default project philosophy: **fail closed, preserve evidence, require explicit operator action for destructive/recovery transitions.**

## 13. Tests to add with a module

At least one of these should protect the new contract:

```text
host C++ test
CMP1 protocol test
static Web audit
release/final acceptance contract assertion
compile/build coverage
hardware regression
```

Do not add a production module with only a happy-path manual check if its invariant can be protected automatically.

## 14. Documentation update after integration

If module ownership/location changes the project map:

```text
update docs/AI_AGENT/01_PROJECT_MAP.md
```

If it creates a new common maintenance task:

```text
update docs/AI_AGENT/02_CHANGE_ROUTER.md
```

If it changes protocol/data/API/hardware contract, update the matching thematic docs.

Only create a new release/handoff checkpoint when project status or release baseline materially changes.

## 15. Definition of integrated module

A module is integrated only when all applicable statements are true:

```text
[ ] exact owner identified
[ ] current production build includes it
[ ] construction lifecycle is explicit
[ ] init failure semantics defined
[ ] periodic work is bounded
[ ] persistence path/schema/recovery defined
[ ] backup/restore coverage updated if persistent
[ ] API contract validated if public
[ ] desktop/mobile parity handled if operator-facing
[ ] safety invariants unchanged or explicitly re-verified
[ ] relevant automated tests added/passed
[ ] required hardware regression passed
[ ] AI map/router updated
```
