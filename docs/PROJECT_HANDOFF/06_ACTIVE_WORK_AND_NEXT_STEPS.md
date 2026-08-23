# Активная работа и следующие шаги

Дата обновления: **2026-08-23**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Current verified GREEN baseline

Последний implementation/test cleanup batch подтверждён GREEN оператором **2026-08-23**: пользователь явно сообщил, что все текущие GitHub Actions зелёные.

Последний implementation/test commit в этом cleanup batch:

```text
bd64e3cc4ba92a6624aed677d98c1620c165013e
test(warehouse): guard against duplicate web bootstrap
```

Он защищает implementation fix:

```text
06a752663504d58ca6908414f8aa8786007c6877
fix(esp32): remove duplicate warehouse web bootstrap
```

Более поздние commits до cleanup checkpoint — documentation/status synchronization; они не меняют firmware/runtime implementation.

## Cleanup status

**SOFTWARE CLEANUP COMPLETE — 100%.**

Full audit A–E, final split-owner sweep, crash-residue classification, source consolidation, regression protection и verification checkpoint завершены. Неразрешённых `DELETE / MERGE / REVIEW` кандидатов в текущей build-included cleanup queue нет.

Hardware two-board ESP32<->Arduino smoke/recovery verification остаётся отдельным physical release gate и не уменьшает software cleanup completion.

## Final cleanup checkpoint

```text
DELETE  no remaining proven cleanup candidates
MERGE   no duplicate authoritative owners remain
KEEP    reviewed live production/build/test/docs/recovery owners
REVIEW  none remain in the named cleanup queue
FIXED   warehouse duplicate Web bootstrap + regression contract
CI      current Actions confirmed GREEN by operator on 2026-08-23
```

Не начинать новый broad zero-debt sweep без конкретного нового source inconsistency, failing test, runtime defect или stale contract evidence.

## Production/safety/provenance closed

- obsolete Arduino parallel `.ino`, old buzzer/start-button owners and stale `CM_Version.h` removed;
- obsolete conductor settings persistence owner removed;
- generated `build/` removed and `.pio/`, `build/` ignored;
- old warehouse wire catalogue/non-paginated spool list removed;
- obsolete calculator helper/static injection removed;
- active migration/recovery owners classified/protected as `KEEP`;
- Arduino/Core state-machine audit and regressions complete;
- UART lost-ACK/timeout/late-`RUN_STARTED` semantics reviewed/hardened;
- exact session/run/spool provenance hardened;
- exact-run finalization coverage and immutable selection required;
- current KG_FIRST store/API/UI requires exact immutable `spool_id`;
- historical `UNALLOCATED` remains read/audit/recovery compatibility evidence only;
- snapshot/state/selection crash-residue policies reviewed by transaction boundary;
- warehouse duplicate Web bootstrap corrected and regression-protected;
- latest cleanup Actions confirmed GREEN by operator.

## Final owner sweep — KEEP

Direct build-included review closed these groups as intentional split owners:

- `RepairRegistry` core/lookup/page/search/similarity;
- `WindingJournalQuery` + `WindingJournalQueryValidation`;
- `WindingJournalTransitionAudit`;
- `WindingJournalSnapshotContext`;
- `RepairCosting` + `RepairCostingValidation`;
- `AutonomousWindingArchive` core/assign/integrity/page;
- `CM_WarehouseLegacySpoolMaterial.cpp` as live migration owner;
- `CM_JobSpoolSelectionLookup.cpp`;
- `CM_WarehouseWriteOffLookup.cpp`;
- current `Core/` and `Arduino/` build-included owner trees.

No deletion was based only on filename or empty GitHub search.

## Warehouse duplicate bootstrap fix

`main.cpp` intentionally calls both:

```text
warehouseWeb.begin();
warehouseWeb.beginSpoolList();
```

Before `06a7526...`, `beginSpoolList()` duplicated common service bootstrap and could register HTTP routes more than once.

Current ownership:

```text
WarehouseWeb::begin()
  common warehouse/write-off/conductor/settings/material services

WarehouseWeb::beginSpoolList()
  GET  /api/warehouse/spools
  POST /api/warehouse/spools/material
  GET  /api/warehouse/material-summary
```

Regression `bd64e3c...` in `Tests/Web/check_warehouse_spool_list_cleanup.js` rejects reintroduction of duplicate bootstrap.

## Crash-residue classification — final

```text
JobStateStore .tmp/.bak
  KEEP fail-closed replacement evidence

JobSpoolSelectionStore .json.tmp
  KEEP bounded recovery before UART boundary

JobSnapshotStore .json.tmp
  KEEP non-authoritative preparation crash evidence; no auto-promote/resume/delete
```

For `JobSnapshotStore`, persistent job/session ID allocation happens before snapshot creation, while authoritative `JobState CREATED` is committed only after successful final snapshot rename/verification. A leftover snapshot `.tmp` therefore cannot become an authoritative job and its session ID is not reused.

## Current production material rule

For every new linked-production manual wire writeoff:

```text
source_session_id + source_run_id + exact immutable spool_id
```

Historical `UNALLOCATED` records are immutable compatibility evidence only. Never recreate optional spool as a post-`RUN_COMPLETED` fallback.

## Safety boundary — never weaken

- no automatic physical START;
- no automatic START between repeat runs;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK/timeout alone never proves Arduino idle;
- final repeat cannot automatically reopen;
- `RUN_COMPLETED` never automatically deducts material;
- writeoff remains explicit/manual and exact session + run + immutable spool;
- cancellation cannot erase immutable run/history evidence;
- restore explicit/operator-only/transactional/fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion or NDJSON truncation.

## Active product work — winding reference integration

Current active product task is adaptation of the legacy winding reference from `FantomeKGZ/motor-winding-reference`, source folders `sourse/desktop` and `sourse/mobile`, into the CoilMaster static site.

Implemented on `cmp-protocol-v1`:

- common CoilMaster visual shell for `/sites/reference/desktop/` and `/sites/reference/mobile/`;
- common desktop/mobile version switch using `cm-ui-version`;
- full CoilMaster navigation from the reference back to workshop sections;
- mobile horizontal navigation so section links remain reachable on phones;
- shared `reference.css` and `reference.js`;
- legacy importer `tools/import_legacy_winding_reference.py`;
- integrity checker `tools/check_legacy_winding_reference.py`;
- CI dry-build workflow `.github/workflows/reference-legacy-import.yml`, which checks out the complete legacy source, imports into a temporary output tree, runs the checker and reports generated footprint.

Confirmed source format/details:

```text
legacy HTML charset: Windows-1251
legacy top banner/logo: div.verh -> images/verh.jpg
sample desktop/mobile 4A.html keeps the same winding tables and internal links
raw exported HTML count before FrontPage filtering: 1856 desktop + 1856 mobile
raw byte-identical resource matches before FrontPage filtering: 5547 per mode
legacy export has no index.html although many pages link to it
```

Importer contract:

- preserve real HTML page content, tables, descriptions and internal links;
- remove only the legacy top `div.verh` banner from published pages;
- output UTF-8 HTML;
- keep desktop/mobile page trees separate;
- deduplicate byte-identical assets into `/sites/reference/shared/assets/` by SHA-256;
- keep unique assets in mode-specific `/desktop/assets/` or `/mobile/assets/`;
- exclude Microsoft FrontPage bookkeeping trees `_vti_*` from both pages and assets;
- route the legacy missing `index.html` home target to the CoilMaster reference shell;
- never modify ESP32/Arduino runtime/safety logic.

### Reference integration — verified artifact

Confirmed GitHub Actions evidence:

```text
Reference Legacy Import Check run 32641366079 — GREEN
head_sha cea85a09dd4208bf88545284ae80a12edce22681
```

Measured output:

```text
desktop pages: 926
mobile pages: 926
catalog entries: 926
pre-existing source gaps: 0
reference size: 329M
full /web size: 331M
reference files: 4626
reference HTML: 1854
shared assets: 2769
full /web files: 4693
```

Published artifact:

```text
coilmaster-web-sd-bundle-cea85a09dd4208bf88545284ae80a12edce22681
artifact id 9493687862
266893049 bytes
expires 2026-09-06
```

Generated desktop/mobile pages now include top and bottom return-to-search navigation, back-to-top access, responsive images and scrollable wide tables. The reference checker requires those shell elements and validates the complete catalog/generated target set.

CMP runs `32640743878` and `32640777993` exposed a committed-tree audit conflict with generated direct links. Commit `116a65a6...` separated that contract. Run `32641366058` exposed a JavaScript regex escaping error, corrected by `e5fc99e6...`.

After `e5fc99e6...`, the operator explicitly confirmed on 2026-08-23 that all current Actions are GREEN. The operator then completed the physical ESP32 `/web` smoke and reported no errors; the verified reference bundle is hardware-smoke GREEN. A follow-up UX batch added the missing `📚 Справочник` entry to the shared desktop/mobile app shell, restored the effective anchor-link audit and made the SD bundle workflow rebuild for every `firmware/esp32/web/**` change. The operator explicitly confirmed all current Actions GREEN. The current follow-up preserves the exact opened generated page when switching desktop/mobile; improves reference search with order-independent multi-token ranking, URL restoration, keyboard/clear controls and quick series filters; adds exact SD web bundle provenance; and improves heavy legacy-page performance/accessibility with lazy image loading plus focusable scroll regions for wide tables. Every new artifact contains `web-bundle-manifest.json`, while both the shared CoilMaster shell and reference pages show `SD <commit>` or fail-soft `SD web unknown`. Verify current CMP and Reference workflows, then deploy the refreshed artifact and smoke the shared-shell entry, same-page switch, representative searches, displayed SD commit, image loading and table scrolling.

Hardware two-board smoke remains a separate external release gate and is not part of this reference-site product task.

## Read order for next chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```


### Reference migration cleanup audit — current batch

Текущая GREEN baseline подтверждена оператором 2026-08-23. Следующий implementation batch добавляет только report-only анализ generated reference:

- `tools/audit_legacy_winding_reference.py`;
- contract test `Tests/Web/check_reference_migration_audit.py`;
- отдельный Actions artifact `coilmaster-reference-migration-audit-<commit-sha>`;
- summary counts для unreferenced assets, byte-identical assets, identical pages и страниц без legacy incoming links.

Политика fail-safe: аудит ничего не удаляет и не меняет в SD bundle. Страницы без legacy incoming links остаются searchable через полный каталог. Проверить новый Reference workflow; после GREEN перенести реальные counts/bytes из audit artifact в integration log и только затем классифицировать кандидатов как KEEP или безопасный служебный мусор.
