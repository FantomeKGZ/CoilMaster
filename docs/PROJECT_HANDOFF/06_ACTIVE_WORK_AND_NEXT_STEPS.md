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

22 августа пользователь предоставил screenshot со свежими GREEN `CMP Protocol Tests` для cleanup/documentation chain вплоть до commit `af43457...` (`Record AI routing cleanup completion`). Это сняло pre-deletion gate для уже dependency-proven warehouse spool-list cleanup. Эти GREEN не считаются ESP32 Build и не заменяют post-deletion firmware build verification.

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
```

### Completed warehouse spool-list cleanup

The old non-paginated spool list path was dependency-audited before deletion. Direct owner inspection proved that current `/api/warehouse/spools` uses only `WarehouseStore::appendActiveSpoolsPageJson()` from `CM_WarehouseSpoolMaterialList.cpp`; no production runtime/operator/persistence contract depends on `appendActiveSpoolsJson()`.

After the fresh user-provided GREEN `CMP Protocol Tests` gate:

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

Status: **DELETE completed; post-deletion ESP32 Build + CMP Protocol Tests pending exact result.**

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

1. verify exact post-deletion result for `ESP32 Build` and `CMP Protocol Tests` on the latest warehouse spool-list cleanup chain;
2. if GREEN, continue ESP32/Arduino owner inventory using direct owner/call-site/build proof rather than empty code-search results;
3. identify the next candidate and classify DELETE / MERGE / KEEP / REVIEW before changing it;
4. continue final root/tree/docs reference sweep as the tree changes;
5. final hardware smoke remains a separate gate and is requested only when runtime behavior needs physical proof.

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
