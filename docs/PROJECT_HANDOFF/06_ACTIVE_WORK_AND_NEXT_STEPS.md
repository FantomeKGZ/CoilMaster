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

Final docs sweep also removed stale navigation references left behind by that cleanup: `docs/AI_AGENT/01_PROJECT_MAP.md` now points to `docs/01_SYSTEM_ARCHITECTURE.md` and `docs/HARDWARE_REFERENCE/` instead of the already removed root `ARCHITECTURE.md` and `Engineering/Hardware/`. Commit: `0649fa769aafb836522127b483e14c998a28cf59`.

AI maintenance routing is now aligned with the active cleanup phase: `docs/AI_AGENT/00_START_HERE.md` records the current `a29e2ab9...` operator-GREEN baseline, marks audit A..E complete and no longer lists removed `CM_ConductorSettings.*` as a future candidate; `docs/AI_AGENT/02_CHANGE_ROUTER.md` routes buzzer work to `CM_BuzzerService.*`, records the removed conductor-settings duplicate, and treats controlled cleanup as the active phase. Commits: `a5d2a42a98bdda0bf0a81736b5b49d2e6c4d9ef6`, `ef2df11179b84a35108967950c39481086c581d9`.

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

## DELETE-ready candidate — waiting for fresh CI gate

```text
firmware/esp32/src/CM_WarehouseSpoolList.cpp
WarehouseStore::appendActiveSpoolsJson()
```

Dependency audit completed on current `cmp-protocol-v1` state:

- `CM_WarehouseSpoolList.cpp` contains only the old non-paginated `appendActiveSpoolsJson()` implementation;
- current `/api/warehouse/spools` owner is `CM_WarehouseSpoolWeb.cpp` and calls only `appendActiveSpoolsPageJson()`;
- the paginated backend is owned by `CM_WarehouseSpoolMaterialList.cpp` and remains KEEP;
- `platformio.ini` compiles all `firmware/esp32/src/*.cpp`, so the old file is still compiled even though it has no production runtime caller;
- `CM_WarehouseStore.h` still contains the obsolete public declaration and must be cleaned in the same deletion batch;
- no runtime path, persistence format, operator workflow or safety contract depends on the old non-paginated API;
- empty GitHub code-search alone is not used as deletion proof; the classification is based on direct owner/header/build checks.

Classification: **DELETE-ready**, but deletion remains blocked until a fresh applicable CI result or explicit operator GREEN for the current pre-deletion cleanup batch. When the gate is available, delete the `.cpp`, remove the declaration from `CM_WarehouseStore.h`, add/update regression coverage, then run ESP32 Build + CMP Protocol/Web audits.

## Calculator — current production contract

```text
source input: one line, up to 5 wires, e.g. 0,51;0,71;0,95
warehouse recommendations: current warehouse catalogue
standard recommendations: read-only IEC 60317 R20 project catalogue
```

Current diameter model is `diameterHundredthsMm` (0.01 mm precision). IEC R20 values are adapted to this precision; moving to 0.001 mm requires an explicit data/model migration.

## Web asset cleanup audit

Current direct loader/owner audit confirms the remaining `firmware/esp32/web/shared/` feature modules are active rather than orphaned cleanup candidates:

- `app-shell.js`, `backup-remote-upload.js`, `costing-pricing-history.js`, `settings-time-status.js` and `settings-system-diagnostics.js` are injected by `CM_StaticSiteServer.cpp` for their matching UI routes;
- `arduino-windings-archive.js`, `backup-restore-stale-guard.js`, `settings-wifi.js`, `settings-hall-calibration.js`, `settings-remote-backup.js`, `winding-history-spools.js` and `writeoff-spool-suggestion.js` have direct current page owners;
- therefore an absent literal `<script>` reference in a page is not sufficient deletion proof because runtime injection is part of the production loader contract.

Classification for the audited shared feature modules: **KEEP**.

`firmware/esp32/web/reference/motor-reference.json` is the deployed generated motor reference dataset and remains **KEEP**.

`firmware/esp32/web/sites/reference/{desktop,mobile}/` is an intentional microSD reference-site shell. It is currently mostly a placeholder, but `docs/07_WEB_PORTAL.md` explicitly retains the architecture for additional sites on microSD with separate desktop/mobile variants. Classification: **KEEP / intentional placeholder**, not cleanup trash. Do not delete merely because current root/desktop navigation does not expose a direct link.

No additional dependency-closed Web asset was proven safe to delete in this pass.

## Persistence resilience item that is NOT cleanup

Direct append tail resilience for new spool/material catalogue records remains a non-blocking P2 design item. Do not auto-truncate malformed/torn production NDJSON; automatic deletion of production evidence is forbidden.

## Next cleanup sequence

1. obtain a fresh GREEN for the current source-deletion/restoration + Web-helper cleanup batch;
2. immediately remove DELETE-ready `CM_WarehouseSpoolList.cpp` + obsolete `appendActiveSpoolsJson()` declaration and protect the removal with regression coverage;
3. continue ESP32/Arduino owner inventory using direct owner/call-site/build proof rather than empty code-search results;
4. Web shared/reference/sites owner audit is complete for the current tree; revisit only if the tree changes or a new candidate appears;
5. final root/tree/docs reference sweep;
6. final applicable CI + hardware smoke remains separate.

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
