# CoilMaster — архитектура развития

## Цель

Зафиксировать текущие правила расширения CoilMaster без возврата удалённых parallel entrypoints, duplicate owners или устаревшей структуры каталогов.

Production source-of-truth branch: `cmp-protocol-v1`.

## Current Arduino source layout

Authoritative build inputs задаёт `platformio.ini`:

```text
Core/*.cpp
Arduino/*.cpp
firmware/arduino/src/main.cpp
```

Текущая структура:

```text
Core/                         realtime/domain state machine and models
Arduino/
├── Config/
│   ├── CM_Pins.h             authoritative Uno pin map
│   └── CM_Features.h         compile-time feature flags
├── CM_DebouncedButton.*
├── CM_HallTurnSource.*
├── CM_SsrController.*
├── CM_Lcd1602View.*
├── CM_BuzzerService.*
├── CM_UartEventTransport.*
├── CM_HardwareControlProtocol.*
├── CM_HallCalibrationProtocol.*
├── CM_HallCalibrationService.*
├── CM_HallTelemetry.*
└── Diagnostics/              standalone hardware diagnostic sketches only

firmware/arduino/src/main.cpp  sole production Arduino composition root
```

Удалённые historical paths `Arduino/CoilMaster_Arduino.ino`, `Arduino/Config/CM_Version.h` и старые parallel buzzer/start-button owners не должны reappear.

## Core / hardware boundary

`Core/` хранит realtime/domain behavior, которое не должно зависеть от конкретной LCD/keypad hardware library.

Hardware adapters в `Arduino/` преобразуют физические сигналы и outputs в контракты state machine:

- keypad/external START -> input events;
- Hall -> turn events;
- state/output model -> LCD/buzzer;
- SSR output остаётся только Arduino-owned и проходит через current safe controller/state machine;
- CMP1 transport связывает accepted remote job и фактические run events с ESP32, но не создаёт physical START.

## ESP32 source layout

Authoritative production build:

```text
firmware/esp32/src/*.cpp
```

Top-level lifetime/composition в основном принадлежит:

```text
firmware/esp32/src/main.cpp
firmware/esp32/src/CM_StaticSiteServer.*
```

Новые service/data/API modules должны иметь одного явного owner: кто constructs, `begin()`/updates, какие routes/files/state они владеют и какие tests защищают contract.

Не добавлять второй persistence owner только потому, что имя/структура кажется удобнее существующего split implementation.

## Web structure

```text
firmware/esp32/web/
├── index.html
├── desktop/
├── mobile/
├── shared/
├── reference/
└── sites/reference/{desktop,mobile}/
```

`reference/motor-reference.json` — generated read-only dataset из `data/motor_catalog/*.source.json`; он не является working motor database и не содержит production `coil_program`.

## Compile-time configuration

Arduino hardware/config availability задаётся в:

```text
Arduino/Config/CM_Features.h
Arduino/Config/CM_Pins.h
```

Uno SRAM/Flash ограничены. Перед добавлением runtime feature проверяются фактическая необходимость, static allocations, frame/queue sizes и PlatformIO resource result. Safety-critical buffers/queues не уменьшаются механически ради нескольких байтов.

## Simulation and diagnostics

`CM_FEATURE_SIMULATION` используется только для безопасного тестирования; при simulation real SSR должен оставаться заблокированным.

Standalone sketches под `Arduino/Diagnostics/` не являются production entrypoints и не должны случайно включаться в PlatformIO production build.

## Persistence development rule

Для нового production `/data/...` path недостаточно только writer-а. Нужно определить:

1. authoritative writer;
2. authoritative reader/parser;
3. corruption/schema validation;
4. identity/cross-reference validation;
5. crash/temp/pending policy;
6. backup/export coverage;
7. restore/rollback/apply behavior;
8. API/UI exposure;
9. regression coverage.

Не добавлять автоматическое удаление/truncation production evidence как recovery shortcut.

## Protocol development rule

Production inter-controller protocol — text CMP1:

```text
Arduino/CM_UartEventTransport.*
firmware/esp32/src/CM_UartEventReceiver.*
Shared/CMP1Text/CM_Cmp1Crc.h
```

Изменение frame grammar требует review обеих плат и соответствующих tests. `Shared/Protocol/` — host-test-only binary protocol и не является replacement для CMP1.

## Change procedure

Перед изменением existing file:

1. fetch текущего содержимого именно из `cmp-protocol-v1`;
2. использовать текущий blob SHA;
3. определить OWNER -> CONTRACT -> PERSISTENCE -> UI/API -> VERIFICATION;
4. менять минимальный coherent file set;
5. обновить regression/doc routing только там, где изменился реальный contract/topology;
6. не объявлять CI/build/hardware GREEN без фактического результата.

Для cleanup каждый кандидат классифицируется:

```text
DELETE / MERGE / KEEP / REVIEW
```

Empty code search — только supporting evidence; direct build/runtime/test owner proof обязателен.

## Version/status source

Репозиторий не использует удалённые `CM_Version.h` или root `CHANGELOG.md` как production version authority. Build identity ESP32 формируется через `scripts/platformio_build_id.py` и соответствующие build flags/runtime endpoint.

Current cleanup/release status и verified GREEN baseline находятся в:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```
