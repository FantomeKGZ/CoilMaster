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
```

Importer contract:

- preserve HTML page content, tables, descriptions and internal links;
- remove only the legacy top `div.verh` banner;
- output UTF-8 HTML;
- keep desktop/mobile page trees separate;
- deduplicate byte-identical assets into `/sites/reference/shared/assets/` by SHA-256;
- keep unique assets in mode-specific `/desktop/assets/` or `/mobile/assets/`;
- never modify ESP32/Arduino runtime/safety logic.

Latest reference fix:

```text
0a126dfd756b592ffdbfb6760f2ba65bf9317f54
fix(reference): resolve generated links from output root
```

The checker previously resolved `/sites/reference/...` through `output.parents[2]`, which was wrong both for the production output path and for the CI temporary output. It now maps the `/sites/reference/` prefix directly to the supplied `--output` root, so valid generated links are checked against the actual generated tree.

Next implementation step: obtain the actual `Reference Legacy Import Check` result for `0a126df...`. If GREEN, use the reported page/link/asset footprint as the import baseline and then commit generated reference content in bounded batches. If it fails, fix the exact reported importer/checker defect first. Do not manually rewrite hundreds of legacy pages and do not call this reference batch GREEN without a real workflow result.

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
