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


### Reference audit CI correction — 2026-08-23

Runs `32643425748` и `32643450801` относятся к superseded regex defect. Runs `32643491396` и `32643578042` дошли до полного reference build и выявили CP1251 legacy stylesheet в report-only scan. Исправление `d7de6693...` добавляет UTF-8/CP1251 decode только для legacy CSS audit; generated HTML UTF-8 gate не ослаблен. Regression `3603b7a2...` воспроизводит CP1251 CSS. Локальный contract test GREEN; дождаться нового Reference run и затем снять реальные audit counts/artifact.


### Reference audit URL accuracy — current batch

Оператор подтвердил GREEN CP1251 correction batch. Текущий follow-up делает cleanup report точнее: учитывает absolute/relative/Windows-style asset URLs и группирует unreferenced candidates по расширению и bytes. Политика остаётся report-only; удалений из generated reference и SD bundle нет. После GREEN workflow использовать новый audit artifact для KEEP/REMOVE классификации по типам файлов.


### Reference global asset dedup — current batch

Оператор подтвердил GREEN URL-accuracy/audit-classification batch. Текущий importer follow-up дедуплицирует все повторяющиеся assets по `SHA-256 + suffix`, включая разные legacy-имена и повторы внутри одного mode. Checker требует отсутствие таких физических дублей во всём generated asset tree. Cross-suffix identical bytes сохраняются раздельно для MIME safety. Contract test подключён к Reference workflow. После GREEN снять обновлённые reference size/file counts и сравнить с verified baseline 329M / 4626 reference files.


### Reference legacy CSS correctness — current batch

Оператор подтвердил GREEN global asset dedup. Текущий follow-up закрывает CSS relative-path boundary: legacy CSS остаётся mode-specific, преобразуется CP1251/UTF-8 -> UTF-8, а локальные `url(...)` переписываются на существующие generated assets. Checker запрещает broken/non-rewritten CSS URLs и CSS внутри content-addressed shared asset tree. Локальный synthetic import + checker GREEN; проверить новый full Reference workflow и затем использовать новый SD artifact.


### Reference CSS Actions correction — current

Runs `32644626781`–`32644735339` являются промежуточными failures mode-safe CSS batch. Последний run уже прошёл importer contracts и полный 926+926 import, но checker не имел `CSS_URL_RE`. Исправление `2df1764a...` добавило matcher; `ef779874...` теперь вызывает CSS checker прямо в раннем synthetic contract. Локальный full import + checker GREEN. Проверить новый Reference run после `ef779874...`; старые шесть runs не rerun.


### Reference embedded CSS — current batch

Оператор подтвердил GREEN CSS matcher/checker correction. Текущий follow-up переписывает и проверяет inline `style`, `<style>`, CSS `url(...)` и quoted `@import`. Exact repository synthetic contract GREEN. После нового Reference GREEN использовать обновлённый artifact; этот блок меняет только generated Web/reference и не затрагивает ESP32/Arduino safety/runtime.


### Reference legacy media attributes — current

Оператор подтвердил GREEN embedded CSS/import batch. Текущий follow-up добавляет `background` и `poster` в importer/checker/audit resource contract вместе с `href/src`. Exact importer/checker/audit tests GREEN. Legacy code search не выявил `srcset`, поэтому отдельный parser не добавлен без source evidence. Проверить новый Reference workflow; runtime/safety не затронуты.

### Reference full content fidelity — current

Оператор подтвердил GREEN legacy media-attribute batch. Commits `e89813f2...` и `fe2aeed1...` добавляют обязательное сравнение normalized visible text и structural counts (`table/tr/th/td/img/a`) для каждой source/generated desktop/mobile страницы. Exact synthetic regression локально GREEN; оператор подтвердил GREEN полного Reference workflow на 926 + 926 страницах.

Оценка готовности reference-сайта: перенос содержимого 100%, общая готовность около 96%. После него: снять размер/число файлов нового artifact, выполнить representative UX/offline checks и physical microSD/ESP32 smoke свежего bundle. Report-only audit по-прежнему ничего автоматически не удаляет.

### Reference exact SD manifest — current

Content-fidelity batch подтверждён GREEN оператором. Текущий follow-up добавляет `tools/build_web_bundle_manifest.py`, regression `Tests/Web/check_web_bundle_manifest.py` и manifest schema v2 с exact counts/bytes и payload-tree SHA-256. Workflow создаёт и немедленно verify-ит manifest до публикации artifact. Локальный positive/tamper contract GREEN; новый Actions batch ещё не объявлен GREEN.

После GREEN перенести exact metrics/hash из workflow summary в integration log, затем выполнить representative desktop/mobile UX и physical microSD/ESP32 smoke свежего artifact. Автоматическое удаление audit candidates по-прежнему запрещено.

### Reference manifest CI correction — current

Runs `32645799868`, `32645815018`, `32645826246`, `32645835821`, `32645845923` упали только на stale `Audit CI trigger coverage`: он искал прежний inline manifest literal после выделения authoritative Python builder. Commit `797e68ae...` проверяет актуальную цепочку workflow arguments + builder provenance + `--verify`. Exact CI-trigger contract локально GREEN; остальные steps указанных runs, включая host build/CTest, были GREEN. Старые runs superseded; проверить новый CMP run.

### Reference complete table/image UX — current

Оператор подтвердил GREEN трёх тестов после manifest CI correction. Текущий batch требует shared runtime/CSS accessibility и performance contracts для всех сохранённых legacy tables/images: horizontal touch/keyboard scrolling, focus region, lazy/async images и fallback alt. Synthetic fixture теперь сначала копирует реальный committed shell, как production workflow. Exact positive/tamper regression локально GREEN; проверить новый Reference workflow. Runtime ESP32/Arduino и safety boundary не затронуты.

### Reference full offline closure — current

Текущий batch добавляет обязательный full-`/web` dependency scan до manifest/upload. Static runtime resources должны быть локальными и существовать; external navigation links и динамические `/api/*` разрешены. Checker покрывает HTML resource attributes, `srcset`, inline/style-block CSS и все CSS files. Positive/remote/missing regressions локально GREEN; оператор подтвердил полный batch GREEN. при failure исправлять конкретные unresolved bundle targets, не ослабляя offline gate автоматически.

### Reference risk-based physical smoke — current

Offline dependency closure подтверждён GREEN оператором; software/site readiness около 98%. Текущий workflow генерирует отдельный `coilmaster-reference-smoke-plan-<sha>` с 4А и наиболее рискованными страницами по tables/images/text/path depth, точными desktop/mobile URLs и bounded physical checklist. Local positive/missing-target contract GREEN. После нового Actions GREEN использовать именно этот plan и свежий SD bundle для завершающего ESP32 smoke.

