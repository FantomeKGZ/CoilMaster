# CoilMaster — полный handoff на 2026-08-08

Ветка: `cmp-protocol-v1`  
Репозиторий: `FantomeKGZ/CoilMaster`

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

## Оценка готовности на текущем этапе

Ориентировочно:

- реализованная функциональность production flow: **≈96%**;
- repository/firmware readiness: **≈93%**;
- общая эксплуатационная готовность: **≈90%**.

До существенно более высокой оценки не хватает фактически подтверждённого ESP32 build текущего HEAD, полного ESP32 + Arduino hardware E2E/fault testing и реального Stage 0 benchmark для performance/rotation решений.

## Production flow — собран

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART delivery → physical START
→ RUN_STARTED/RUN_COMPLETED → manual wire writeoff
→ source_session_id + source_run_id → materials/pricing
→ finalization preflight → CLOSED → archive/report → read-only backup
```

Не проектировать этот flow заново.

Safety invariants:

- никакого automatic physical START;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
- `RUN_COMPLETED` сам по себе не списывает провод;
- wire writeoff остаётся ручным и связан с exact `spool_id`, `source_session_id + source_run_id`;
- corruption/storage loss fail closed.

Hardware E2E считается выполненным только после реального ESP32 + Arduino стенда.

## Production web bootstrap — теперь соответствует реализованным модулям

Текущий `main.cpp` гарантированно вызывает `WarehouseWeb::begin()`. Через этот bootstrap теперь реально зарегистрированы:

```text
/api/warehouse/summary
/api/warehouse/spools
/api/warehouse/price
/api/warehouse/write-offs GET/POST
/api/materials*
/api/repairs/costing GET/POST
/api/repairs/pricing-history
/api/calculator
/api/calculator/settings GET/POST
```

Workshop/repair/similarity/backup routes продолжают регистрироваться штатными существующими web modules.

Ключевые commits:

```text
961229e410b91b788984a2ee14a1a8b4f1276073  Register warehouse writeoff routes in production bootstrap
74f5bea9273ab47d5df1c4ef2f0557445b21a6ed  Register production material and conductor APIs
```

Writeoff handler при отсутствии переданного `JobSpoolSelectionStore*` создаёт fallback store и всё равно проверяет exact session → repair/spool selection. Это не correctness gap, но может быть performance cleanup candidate после benchmark.

## Winding journal authority

Deep backup/finalization/manual-writeoff proof используют:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

`validateAll()` теперь действительно строгий authority:

- full flat JSON syntax;
- schema 1/2 fields;
- canonical unsigned values;
- single semantic `event` key;
- single nullable `repair_id`/`motor_id` keys для schema 2;
- linked/unlinked shape;
- record counter overflow guard.

Startup `WindingJournal::begin()` также проверяет full flat JSON syntax внутри уже существующего `validateJournalStructure()` pass — без нового startup full scan.

Finalization больше не делает full integrity scan через cursor-pagination temporary JSON pages. Manual writeoff не принимает изолированную `RUN_COMPLETED`, если full schema/transition integrity нарушена.

Ключевые commits:

```text
f3e02dadce34c7a85e11d23154652a469d6aa111  Make winding validateAll reject malformed JSON
796724367eef66a3e2ba2b95c90cb03df641ac73  Reject duplicate winding event keys in query parser
b3b12496837721f83895c93799e14b007f584869  Reject duplicate winding event keys in transition audit
d92b6f1c19a5d0c5c2abb3663410e9202bcd385e  Use authoritative winding validation for finalization
9e18cd076b8e1ad904e3893aab20cde1a4e596a8  Require authoritative winding integrity before writeoff
d7c601fe18d8b60794334cb2572e40a7777cc0d2  Reject malformed winding JSON during startup validation
```

## Flat persisted JSON hardening

Shared helper:

```text
firmware/esp32/src/CM_FlatJsonObjectValidator.h
```

Он предназначен только для persisted flat JSON objects с primitive values. Не использовать его как замену специальных parsers файлов с arrays/nested structures, например job snapshot `turns`.

Усилены основные startup/runtime/history/audit paths workshop, costing, material, warehouse, settings и winding.

Особенно важно:

- `RepairLifecycle` strict для `repair-status.ndjson`;
- `JobLinkageResolver` strict до выделения job/session ID;
- spool-selection immutable parser strict;
- costing/pricing/material/writeoff histories fail closed;
- exact active spool/warehouse price/runtime summaries strict;
- material and costing repair references strict;
- warehouse repair legacy bool lookup strict;
- material calculator warehouse catalogue strict.

## MaterialLedger — прежний крупный runtime gap закрыт

Commit:

```text
c4ff5ff4d195959c3fc70bd8e0b8bf72ac5a0525  Fail closed on malformed material ledger runtime state
```

Strict validation теперь покрывает:

- material catalog output;
- material usage preflight scan;
- pending usage metadata + audit line;
- pending recovery identity `usage_id/material_id`;
- durable usage scan;
- stock lookup;
- next ID scan;
- quantity consume/restore atomic rewrite rows;
- rewritten row до temp-file write.

Broad persistence audit перед каждой mutation не добавлен, поэтому correctness hardening не превращён в дополнительный SD full scan.

## Conductor settings — production authority исправлен

Production server-authoritative файл:

```text
/data/settings/conductor.json
```

Ранний `/data/settings/conductor-calculator.ndjson` был промежуточным store и не является текущим production contract.

Теперь:

- runtime `conductor.json` проходит strict JSON validation;
- `allow_mixed_diameters` реально сохраняется;
- старый `conductor.json` без поля backward-compatible трактуется как `true`;
- deep audit проверяет production `conductor.json`;
- наличие legacy `conductor-calculator.*` считается ambiguous persistence и блокирует stable deep audit;
- backup whitelist экспортирует `conductor.json`;
- calculator warehouse catalogue fail closed на malformed spool rows.

Ключевые commits:

```text
6cde2831f190918f102ca39eb9f6b3687a7602ea  Persist and validate mixed conductor setting
932624e5a83187eca6545182fefb3cfd81f0bc77  Audit production conductor settings file
012cd0816a53489bb6169b69f3079f6ce46f9de5  Export production conductor settings file
b006e77152777873a756f2356d5b9546d821dada  Reject malformed calculator wire catalogue rows
```

## Deep backup integrity coverage

Safe `snapshot_stable=true` выполняется только при `BackupActivityGuard::Safe` и требует integrity:

- persistent allocator main/backup, no allocator temp residue;
- production conductor settings;
- workshop clients/motors/repairs + repair-status;
- repair pricing + references;
- material catalog/usage/adjustments + recovery state;
- winding journal full schema + transition semantics;
- warehouse spools/price/movements + references;
- canonical snapshot/state/spool-selection directories;
- contents всех session files штатными parsers;
- cross-file job/session/repair/motor/spool identity.

При active winding deep scan/export блокируется.

## Stage 0 performance observability — 29 metrics

Manifest уже достаточно подробен для первого реального benchmark. Не добавлять telemetry ради количества.

Измеряются:

- total snapshot stability duration;
- 9 per-domain audit durations;
- allocator high-water;
- material/business/winding/warehouse record counts;
- snapshot/state/spool-selection file counts;
- aggregate session bytes.

Все metrics собираются внутри существующих authoritative passes или вокруг них, без отдельного telemetry full scan.

Known hotspots, которые пока только измерять:

1. material audit transitive workshop/pricing scans;
2. business uniqueness/reference repeated scans;
3. warehouse reference repeated scans;
4. preliminary session-directory scan перед deep session audit;
5. `WireWriteOffCoverageAudit` cursor pages + repeated movement scans;
6. writeoff fallback spool-selection `begin()`/directory audit.

До benchmark не вводить database migration, persistent optimistic cache, arbitrary rotation threshold или Stage 1 index refactor.

## CI / verification

Последняя исторически подтверждённая compile failure была missing namespace closing brace в `CM_MaterialLedger.cpp`, исправленная:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8
```

Для последних commits GitHub connector пока не показывает combined statuses/workflow runs. Это **CI NOT CONFIRMED**, не GREEN.

Static diff review подтвердил, что большие whole-file updates не потеряли соседнюю логику:

- backup whitelist commit меняет только conductor export definition;
- WindingJournal startup commit меняет только include + structural syntax check;
- WarehouseWeb bootstrap commit меняет только includes + service registration;
- MaterialLedger commit меняет заявленные validation/recovery paths.

## Точная следующая repo-only точка

1. Проверить реальный Actions/ESP32 build текущего HEAD, если run появится в connector.
2. Найти точную implementation unit tri-state `WarehouseStore::repairExists(uint32_t, bool&)` через подтверждённый branch path/history и проверить fail-closed semantics. Не создавать второй implementation по предположению.
3. Продолжать repo-only только при конкретной correctness/compile причине.
4. После фактического benchmark выбрать самый дорогой `*_duration_ms` и только тогда делать bounded index/audit decomposition/rotation.

## Обязательный hardware E2E

```text
linked repair
→ exact spool
→ JOB_ACK
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ manual wire writeoff
→ costing
→ finalization preflight
→ CLOSED
→ stable backup
```

Fault cases:

- reboot/manual-review;
- microSD loss;
- corrupted persistence;
- UART reject/timeout/duplicate events;
- wrong session/run/spool writeoff;
- duplicate writeoff;
- close without wire coverage;
- backup during active winding.

На стенде сохранить один `/api/backup/manifest` с:

```text
items[].size_bytes
всеми 29 Stage 0 metrics
snapshot_stability_duration_ms
```

Только после успешного полного E2E + подтверждённой сборки + benchmark эксплуатационную готовность можно поднимать существенно выше 90%.
