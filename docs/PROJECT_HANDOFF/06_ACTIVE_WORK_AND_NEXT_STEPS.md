# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Verification baseline

Последний явно подтверждённый пользователем GREEN state перед текущим cleanup batch:

```text
a29e2ab9639181550ba2beccf812320a552eb8c8
Remove superseded CMP core dependency package
USER CONFIRMED GREEN
```

22 августа пользователь предоставил screenshot со свежими GREEN `CMP Protocol Tests` для cleanup/documentation chain вплоть до commit `af43457...` (`Record AI routing cleanup completion`). Это сняло pre-deletion gate для уже dependency-proven warehouse spool-list cleanup.

После удаления old spool-list backend пользователь предоставил новый screenshot, который подтверждает:

```text
ESP32 Build GREEN
  commit 5d5d7937a16288f534e3b42047359cc8b65db386
  Remove obsolete spool list API declaration

CMP Protocol Tests GREEN
  commit 7f93db3c330e6f16cd91702df0faa532dc478355
  Fix warehouse spool cleanup regression literals

CMP Protocol Tests GREEN
  commit 60457072e9ecbf74b7386e4800dcca9c37b65541
  Record spool cleanup CI regression fix
```

Так как `5d5d793...` идёт после удаления `CM_WarehouseSpoolList.cpp` (`e24e1dc...`) и obsolete declaration, этот ESP32 Build является применимым post-deletion firmware build gate. Свежие GREEN `7f93db3...` и `6045707...` подтверждают исправленный regression contract. Warehouse spool-list cleanup теперь CI-verified; hardware proof для этого dead-code deletion не требуется.

Старый run `32561236301` полностью разобран: единственной ошибкой был regression-test, который пытался открыть уже удалённый legacy `CM_ConductorSettings.h`; второй скрытой ошибки не было. Исправление — `51ea46c1823a451e7f80ecd188daf896aafc752d`, после него пользователь подтвердил GREEN.

## Full-code audit status

Repo-review/source-contract этапы завершены:

```text
A Arduino safety/realtime/UART/resources                  COMPLETE
B ESP32 runtime/API/persistence/integrity/network/backup COMPLETE
C desktop/mobile/shared Web parity/error/security        COMPLETE
D tests/CI/build-filter/path-trigger audit               COMPLETE
E docs/AI routing consistency                            COMPLETE
```

Authoritative detail: `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`.

## Current active phase — controlled cleanup

```text
DELETE  proven unused and unreferenced by production/build/tests/docs/runtime
MERGE   duplicate implementation; retain one authoritative owner
KEEP    active production/build/test/docs/history/operator dependency
REVIEW  uncertain dependency; do not delete
```

Не удалять файл только из-за имени `Legacy`: migration/recovery compatibility может быть active production contract. Пустой GitHub code-search сам по себе не является достаточным доказательством отсутствия call-site.

## Cleanup completed

Удалены доказанные legacy/generated/duplicate artifacts:

```text
legacy CM_ConductorSettings.* implementation + obsolete source-contract
empty .github placeholder README files
empty LICENSE placeholder
CONTINUE_CMP_PROTOCOL_V1.md
REGISTER.md
CHANGELOG.md
TASKBOOK.md
BUILD_INFO.md
old root ARCHITECTURE.md after merge into docs/01_SYSTEM_ARCHITECTURE.md
capitalized Docs/ legacy documentation
tracked generated build/ output; build/ added to .gitignore
old Arduino CM_Buzzer.* and CM_BuzzerController.*
old Arduino CM_StartButton.*
Engineering/ legacy documentation layer
Tests/README.md Build-002A documentation artifact
old untyped warehouse wire catalogue API/CM_WarehouseWireCatalogue.cpp
firmware/esp32/web/shared/calculator-multisource.js
  + its obsolete CM_StaticSiteServer calculator injection
firmware/esp32/src/CM_WarehouseSpoolList.cpp
  + obsolete WarehouseStore::appendActiveSpoolsJson() declaration
Arduino/CoilMaster_Arduino.ino
  obsolete parallel Arduino IDE entrypoint; production owner is firmware/arduino/src/main.cpp
Arduino/Config/CM_Version.h
  stale 0.2.0-arduino-core-dev header with no current production owner
```

### Arduino config cleanup — stale version header

`Arduino/Config/CM_Version.h` contained only the obsolete development label `0.2.0-arduino-core-dev`. Direct owner/build inspection showed that current Arduino production composition is defined by `PROJECT.manifest`, `platformio.ini` and `firmware/arduino/src/main.cpp`; that production entrypoint does not include the version header and no current source contract consumes its version macros.

Cleanup chain:

```text
8a97cf7170ce872b1c513886ff07c239e7f3c26b
  remove Arduino/Config/CM_Version.h

5e0c788fa43e51a9a6fc5d26631f03aea78e9495
  extend Tests/Protocol/check_arduino_entrypoint_cleanup.js so both the old
  parallel .ino and stale version header must remain absent
```

`Arduino/Config/CM_Features.h` and `CM_Pins.h` remain **KEEP**: they are current production compile-time/hardware configuration included by the authoritative Arduino entrypoint.

Status: **DELETE completed; fresh applicable CI for `5e0c788...` is not yet recorded here.**

### Completed Arduino parallel-entrypoint cleanup — CI VERIFIED

`Arduino/CoilMaster_Arduino.ino` was directly compared with current production composition and build ownership before deletion.

Proof:

- `platformio.ini` compiles `Core/*.cpp`, `Arduino/*.cpp` and `firmware/arduino/src/main.cpp`; the `.ino` is not part of the production build;
- current `firmware/arduino/src/main.cpp` owns UART event transport, EEPROM persistence, hardware settings/control, Hall calibration/telemetry, remote JOB handling and current diagnostics;
- the old `.ino` lacked those production paths and already diverged from current buzzer/state behavior, so retaining it as a second Arduino entrypoint could mislead future maintenance/operator work;
- no exact current repository reference to `CoilMaster_Arduino.ino` was found; deletion classification does not rely on search alone, but on the direct build/owner comparison above.

Cleanup chain:

```text
857f5b21299a04fdc6a80b764609dfee6c7e0b01
  remove Arduino/CoilMaster_Arduino.ino

cd2d3311d9f2a56a537ad27ba9149073d64a10ff
  add Tests/Protocol/check_arduino_entrypoint_cleanup.js

6b036f04e3b82f8af7450df1b37abb35cb75f0cb
  run authoritative Arduino entrypoint contract in CMP Protocol Tests
```

Regression contract requires the obsolete `.ino` to remain absent and preserves `firmware/arduino/src/main.cpp` as the PlatformIO production entrypoint with current UART/persistence/hardware-control composition.

The first post-deletion Uno build run `32573351876` failed after successful compilation/linking only at PlatformIO size validation:

```text
Flash 33156 / 32256 bytes = 102.8%  (+900 bytes)
SRAM   2044 / 2048 bytes  = 99.8%  (4 bytes static headroom)
```

This was a real pre-existing production-resource problem exposed by the fresh build, not a dependency on the deleted `.ino`.

Resource recovery chain:

```text
82ab5d8bc822d4415db77c29a676cf8d14ecbbcb
  expose retained Hall calibration result access

ef33cb8b09e737f5d02d47f4fd6e6e0417335d1f
  compile disabled human-readable USB diagnostics out of the production Uno image
  and remove duplicate calibration-result state; CMP1 remains on SoftwareSerial

c361214c0b89d64ea537c75bcb180ea8e82e20c5
  transient pointer-style GET accessor; CMP tests passed but Uno build failed compatibility

de70cce10664636da0a6686535693887da1ad695
  restore bounded compatible calibration result accessor
```

Operator screenshot confirms on `de70cce...`:

```text
Arduino Uno Build GREEN
CMP Protocol Tests GREEN
```

The screenshot does not expose exact post-fix Flash/SRAM byte counts, so no new numerical headroom is inferred. Resource margin remains worth monitoring, but the old 4-byte SRAM condition must not be treated as current after the GREEN recovery.

Status: **DELETE completed and CI VERIFIED.**

The remaining `Arduino/Diagnostics/CM_Lcd1602CyrillicTest/CM_Lcd1602CyrillicTest.ino` is **KEEP / diagnostic tool**: it is an intentionally standalone LCD character-ROM probe and explicitly does not initialize/control SSR, motor, Hall, UART JOB, or START.

### Completed warehouse spool-list cleanup — CI VERIFIED

The old non-paginated spool list path was dependency-audited before deletion. Direct owner inspection proved that current `/api/warehouse/spools` uses only `WarehouseStore::appendActiveSpoolsPageJson()` from `CM_WarehouseSpoolMaterialList.cpp`; no production runtime/operator/persistence contract depends on `appendActiveSpoolsJson()`.

Cleanup chain:

```text
e24e1dc60764798d0fa27e96561bb8bf179e3eb3
  remove firmware/esp32/src/CM_WarehouseSpoolList.cpp

5d5d7937a16288f534e3b42047359cc8b65db386
  remove obsolete appendActiveSpoolsJson() public declaration

20f76c6f3c55871911658170c17f64ba3f793924
  add Tests/Web/check_warehouse_spool_list_cleanup.js

fcc0617d435249b3031728913cdc259a682d6dd9
  run the new cleanup contract in CMP Protocol Tests
```

The regression contract requires the old `.cpp` and API declaration to remain absent while `/api/warehouse/spools` remains bound to the paginated/material-aware backend with cursor/limit/has_more/next_cursor semantics.

Two post-cleanup `CMP Protocol Tests` runs failed only in the newly added static contract test:

```text
32572726225  commit fcc0617...  FAILED step: Audit warehouse spool list cleanup contract
32572752127  commit cac3915...  FAILED step: Audit warehouse spool list cleanup contract
```

In both runs, CMake build + all 3 host tests passed, and every other Web/safety contract step passed. The failing test incorrectly searched C++ JSON string fields `has_more` / `next_cursor` using quote escaping that did not match the literal source representation. Production `/api/warehouse/spools` already contained both fields and still used `appendActiveSpoolsPageJson()`.

Fix:

```text
7f93db3c330e6f16cd91702df0faa532dc478355
Fix warehouse spool cleanup regression literals
```

The test now checks stable field tokens without depending on C++ string-literal escaping. No production firmware/API rollback was made.

Verified results supplied by operator:

```text
ESP32 Build GREEN        5d5d793...
CMP Protocol Tests GREEN 7f93db3...
CMP Protocol Tests GREEN 6045707...
```

Status: **DELETE completed and CI VERIFIED.**

### Documentation routing cleanup

`docs/AI_AGENT/01_PROJECT_MAP.md` now points to `docs/01_SYSTEM_ARCHITECTURE.md` and `docs/HARDWARE_REFERENCE/` instead of removed root `ARCHITECTURE.md` and `Engineering/Hardware/`. Commit: `0649fa769aafb836522127b483e14c998a28cf59`.

`docs/AI_AGENT/00_START_HERE.md` and `02_CHANGE_ROUTER.md` now reflect completed audit A..E, current controlled cleanup, `CM_BuzzerService.*`, removed duplicate conductor settings, and the direct-owner proof rule. Commits: `a5d2a42a98bdda0bf0a81736b5b49d2e6c4d9ef6`, `ef2df11179b84a35108967950c39481086c581d9`.

Calculator helper cleanup is regression-protected by `Tests/Web/check_calculator_source_wire_input.js`: the old helper file and StaticSiteServer injection must remain absent while the `sourceWires` UI remains authoritative.

Полезная binary host-test CMP документация из старого `Docs/` сохранена рядом с owner в `Shared/Protocol/README.md`.

`data/motor_catalog/` проверен: это реальный reference catalogue, не runtime мусор — KEEP.

`scripts/` и `tools/` проверены: build-id generator и motor-reference build/check utilities активны — KEEP.

## Cleanup correction — active legacy spool migration KEEP

Во время source cleanup `CM_WarehouseLegacySpoolMaterial.cpp` был ошибочно классифицирован как unused из-за ненадёжного GitHub code index. Прямой owner-check показал, что `CM_WarehouseSpoolWeb.cpp` регистрирует:

```text
POST /api/warehouse/spools/material
```

и вызывает `WarehouseStore::assignLegacySpoolMaterial()` для безопасного назначения `CU/AL` старой ACTIVE катушке без `wire_type`.

Поэтому API и implementation восстановлены:

```text
d334dcc09c96afdee707b94ffe52611069be0ec3  restore public API
75a34f1c23c30fdf0606b9577ea6eec549bcf210  restore implementation
```

Classification: **KEEP**.

Этот dependency защищён `Tests/Web/check_warehouse_legacy_spool_material_contract.js`: endpoint, public method, ACTIVE/unknown-material guard и atomic `replaceSpoolsFileFromTemp()` должны оставаться согласованными.

## Confirmed KEEP source clusters

```text
CM_WarehouseMaterialCatalogue.cpp
  material-specific CU/AL catalogue used by ConductorCalculatorWeb

CM_WarehouseSpoolMaterialList.cpp
  paginated material-filtered /api/warehouse/spools backend

CM_WarehouseLegacySpoolMaterial.cpp
  active old-data migration endpoint; not dead code

CM_MaterialHistory.cpp + CM_MaterialUsageHistory.cpp
  separate active adjustments and usage journals

CM_JobDisplayRecovery.*
  active immutable-snapshot display recovery; no UART/SSR side effects

Arduino/CM_HallCalibrationProtocol.*
Arduino/CM_HardwareControlProtocol.*
Arduino/CM_HallCalibrationService.*
Arduino/CM_HallTelemetry.*
  current Hall calibration/settings/telemetry stack; directly owned by UartEventTransport + production main

Arduino/Config/CM_Features.h + Arduino/Config/CM_Pins.h
  authoritative production compile-time/hardware configuration

Arduino/Diagnostics/CM_Lcd1602CyrillicTest/CM_Lcd1602CyrillicTest.ino
  standalone LCD character-ROM diagnostic; no production motor/SSR/UART/START ownership

PROJECT.manifest
  current source/boundary manifest
```

## Calculator — current production contract

```text
source input: one line, up to 5 wires, e.g. 0,51;0,71;0,95
warehouse recommendations: current warehouse catalogue
standard recommendations: read-only IEC 60317 R20 project catalogue
```

Current diameter model is `diameterHundredthsMm` (0.01 mm precision). IEC R20 values are adapted to this precision; moving to 0.001 mm requires an explicit data/model migration.

## Web asset cleanup audit

Current direct loader/owner audit confirms the remaining `firmware/esp32/web/shared/` feature modules are active rather than orphaned cleanup candidates. Runtime injection from `CM_StaticSiteServer.cpp` is part of the production loader contract, so absence of a literal page `<script>` reference is not deletion proof.

`firmware/esp32/web/reference/motor-reference.json` remains **KEEP**.

`firmware/esp32/web/sites/reference/{desktop,mobile}/` remains **KEEP / intentional placeholder** under the documented multi-site microSD architecture.

No additional dependency-closed Web asset was proven safe to delete in that pass.

## Persistence resilience item that is NOT cleanup

Direct append tail resilience for new spool/material catalogue records remains a non-blocking P2 design item. Do not auto-truncate malformed/torn production NDJSON; automatic deletion of production evidence is forbidden.

## Next cleanup sequence

1. verify fresh applicable CI for the stale Arduino version-header cleanup (`5e0c788...` and later);
2. continue ESP32/Arduino owner inventory using direct owner/call-site/build proof rather than empty code-search results;
3. audit Uno resource margin without weakening safety/features; exact post-fix bytes should be recorded if a successful build log is available;
4. identify the next candidate and classify DELETE / MERGE / KEEP / REVIEW before changing it;
5. add/update regression coverage before or with any deletion that could otherwise silently regress;
6. continue final root/tree/docs reference sweep as the tree changes;
7. final hardware smoke remains a separate gate and is requested only when runtime behavior needs physical proof.

## External hardware verification gate

Targeted ESP32<->Arduino smoke remains required when the stand is available but does not block cleanup of proven-unused software assets:

```text
normal JOB -> Arduino READY
physical START only
RUN_STARTED -> RUN_COMPLETED
repeat > 1 -> physical START each run
no automatic material writeoff
zero-run cancel / ALREADY_CLEAR / safe ALL_CLEAR
late zero-id ALL_CLEAR must not cancel fresh job
lost JOB_ACK -> timeout/manual review -> late RUN_STARTED reconciliation
reboot waiting/running -> no auto resume
```

Hardware GREEN is not inferred from CI.

## Safety boundary

Never change during cleanup:

- physical START only physical/local;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web do not drive SSR directly;
- timeout/lost ACK alone never proves Arduino idle;
- final repeat cannot reopen automatically;
- RUN_COMPLETED never auto-writes off material;
- manual writeoff uses exact source_session_id + source_run_id;
- KG_FIRST spool omission only approved unallocated/manual path;
- exact spool provenance when spool is used;
- backup/restore explicit, operator-only, transactional/fail-closed;
- no automatic production-data deletion.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/66_ESP32_BUILD_ID_CI_RECOVERY_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```