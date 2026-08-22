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

Commits после `a29e2ab9...` не считаются GREEN автоматически и требуют нового exact workflow result или явного подтверждения пользователя.

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

Не удалять файл только из-за имени `Legacy`: migration/recovery compatibility может быть active production contract.

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
```

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

Поэтому API и implementation восстановлены из exact previous blob:

```text
d334dcc09c96afdee707b94ffe52611069be0ec3  restore public API
75a34f1c23c30fdf0606b9577ea6eec549bcf210  restore implementation
```

Classification: **KEEP**. В дальнейшем пустой GitHub code-search не считать достаточным доказательством отсутствия call-site.

Этот dependency теперь защищён отдельным `Tests/Web/check_warehouse_legacy_spool_material_contract.js` и workflow step: endpoint, public method, ACTIVE/unknown-material guard и atomic `replaceSpoolsFileFromTemp()` должны оставаться согласованными.

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

## Current REVIEW candidates

```text
firmware/esp32/src/CM_WarehouseSpoolList.cpp
  old non-paginated appendActiveSpoolsJson(); current /api/warehouse/spools uses
  appendActiveSpoolsPageJson(). Do not delete until a direct second call-site check and compile gate.
```

## Calculator — current production contract

```text
source input: one line, up to 5 wires, e.g. 0,51;0,71;0,95
warehouse recommendations: current warehouse catalogue
standard recommendations: read-only IEC 60317 R20 project catalogue
```

Current diameter model is `diameterHundredthsMm` (0.01 mm precision). IEC R20 values are adapted to this precision; moving to 0.001 mm requires an explicit data/model migration.

## Persistence resilience item that is NOT cleanup

Direct append tail resilience for new spool/material catalogue records remains a non-blocking P2 design item. Do not auto-truncate malformed/torn production NDJSON; automatic deletion of production evidence is forbidden.

## Next cleanup sequence

1. obtain a fresh GREEN for the current source-deletion/restoration + Web-helper cleanup batch before further uncertain C++ deletion;
2. continue ESP32/Arduino owner inventory, using direct owner/call-site proof rather than empty code-search results;
3. check remaining Web assets for direct links/runtime injection and remove only dependency-closed dead assets;
4. final root/tree/docs reference sweep;
5. final applicable CI + hardware smoke remains separate.

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
