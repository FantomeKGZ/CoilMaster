# Где остановились и что делать дальше

Дата обновления: 2026-08-08  
Ветка: `cmp-protocol-v1`

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

Полный snapshot: `docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md`.

## Текущая оценка готовности

Ориентировочно на текущем HEAD:

- реализованная функциональность production flow: **около 96%**;
- repository/firmware readiness: **около 93%**;
- общая эксплуатационная готовность: **около 90%**.

Последний показатель намеренно ниже: текущий ESP32 HEAD ещё не имеет подтверждённого GREEN Actions result, полный ESP32 + Arduino hardware E2E не выполнен, а Stage 0 performance metrics ещё не сняты на реальном production dataset.

## Production flow — собран и web-routes зарегистрированы

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual wire writeoff
→ source_session_id + source_run_id → materials/pricing
→ finalization preflight → CLOSED → archive/report → read-only backup
```

Текущий production web bootstrap реально регистрирует:

```text
workshop clients/motors/repairs
motor similarity
warehouse summary/spools/price
warehouse writeoff GET/POST
materials CRUD/usage/adjustments
repair costing/pricing history
conductor calculator
conductor settings
backup manifest/file/session export
```

Ключевые bootstrap commits:

```text
961229e410b91b788984a2ee14a1a8b4f1276073  Register warehouse writeoff routes in production bootstrap
74f5bea9273ab47d5df1c4ef2f0557445b21a6ed  Register production material and conductor APIs
```

`WarehouseWeb::begin()` является гарантированно вызываемым production bootstrap point из текущего `main.cpp`. Material/costing/conductor handlers остаются fail-closed, если соответствующий store/recovery недоступен.

## Safety invariants — не менять

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- automatic resume после reboot отсутствует;
- `RUN_COMPLETED` сам по себе не выполняет wire writeoff;
- wire writeoff остаётся ручным;
- writeoff связан с exact `spool_id`, `source_session_id + source_run_id`;
- corrupted persistence/storage loss блокирует опасную операцию fail-closed.

## Winding / finalization / writeoff integrity — текущий уровень

Authoritative full winding validation:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

`validateAll()` теперь проверяет полный flat JSON syntax, schema fields, single `event`, single nullable `repair_id/motor_id`, canonical numeric values и linkage shape.

Ключевые commits:

```text
f3e02dadce34c7a85e11d23154652a469d6aa111  Make winding validateAll reject malformed JSON
796724367eef66a3e2ba2b95c90cb03df641ac73  Reject duplicate winding event keys in query parser
b3b12496837721f83895c93799e14b007f584869  Reject duplicate winding event keys in transition audit
d92b6f1c19a5d0c5c2abb3663410e9202bcd385e  Use authoritative winding validation for finalization
9e18cd076b8e1ad904e3893aab20cde1a4e596a8  Require authoritative winding integrity before writeoff
d7c601fe18d8b60794334cb2572e40a7777cc0d2  Reject malformed winding JSON during startup validation
```

Startup `WindingJournal::begin()` теперь выполняет full flat-JSON syntax proof внутри уже существующего `validateJournalStructure()` pass. Дополнительного startup full scan ради syntax не добавлено; повторные save-path helper scans намеренно не получили parser multiplier.

Manual wire writeoff completion proof требует full schema + transition integrity до проверки exact completed `(session_id, run_id)`.

Finalization/CLOSED больше не строит временные cursor-pagination JSON pages ради integrity scan; используется authoritative EOF validation.

## Flat persisted JSON runtime hardening

Shared primitive-flat-object validator:

```text
firmware/esp32/src/CM_FlatJsonObjectValidator.h
```

Он используется в основных backup/startup/runtime/history/writeoff readers workshop/material/warehouse/settings/winding. Это не новый filesystem pass: parser применяется к уже прочитанной строке.

Крупный ранее открытый `CM_MaterialLedger.cpp` теперь закрыт:

```text
c4ff5ff4d195959c3fc70bd8e0b8bf72ac5a0525  Fail closed on malformed material ledger runtime state
```

Теперь strict JSON validation применяется в:

- material catalog JSON output;
- material scan перед usage confirmation;
- pending usage metadata + audit line recovery;
- durable usage lookup;
- stock lookup;
- generic next-ID scan;
- atomic quantity rewrite/restore source rows;
- rewritten row проверяется до temp-file write.

Pending usage recovery дополнительно требует совпадения `usage_id/material_id` metadata и audit line.

## Conductor settings — authoritative production path

Реальный server-authoritative settings file:

```text
/data/settings/conductor.json
```

Ранний `ConductorSettingsStore` с `/data/settings/conductor-calculator.ndjson` был промежуточной реализацией и не является production contract.

Исправлено:

- active runtime store strict JSON;
- `allow_mixed_diameters` теперь действительно сохраняется;
- старый `conductor.json` без этого поля читается backward-compatible как `true`;
- deep settings audit проверяет `conductor.json`;
- наличие legacy `conductor-calculator.*` считается ambiguous persistence и делает deep audit fail-closed;
- backup whitelist экспортирует `conductor.json`, а не experimental NDJSON;
- warehouse-aware calculator catalog fail-closed на malformed spool rows.

Ключевые commits:

```text
6cde2831f190918f102ca39eb9f6b3687a7602ea  Persist and validate mixed conductor setting
932624e5a83187eca6545182fefb3cfd81f0bc77  Audit production conductor settings file
012cd0816a53489bb6169b69f3079f6ce46f9de5  Export production conductor settings file
b006e77152777873a756f2356d5b9546d821dada  Reject malformed calculator wire catalogue rows
```

## Stage 0 backup observability — достаточно, не расширять ради количества

Manifest возвращает 29 runtime metrics без telemetry-only full scan:

- total snapshot stability duration;
- 9 per-domain durations;
- allocator high-water;
- material/business/winding/warehouse record counts;
- session snapshot/state/spool-selection counts;
- session byte totals.

Сохраняются правила:

- `BackupActivityGuard::Safe` gating;
- no deep scan while winding active;
- failed-domain duration может публиковаться, неисполненные последующие domains остаются `null`;
- partial domain counts/high-water не публикуются;
- session byte overflow telemetry-only и не меняет integrity result.

Не добавлять новые metrics до benchmark.

## Известные performance hotspots — пока измерять

- transitive material → workshop/pricing integrity scans;
- business uniqueness/reference repeated scans;
- warehouse reference repeated scans;
- preliminary session-directory scan до authoritative session audit;
- `WireWriteOffCoverageAudit`: cursor pages + repeated movement scans для completed runs;
- writeoff fallback spool-selection store делает directory audit, когда `WarehouseWeb` не получил готовый `JobSpoolSelectionStore*`.

Последний пункт не ослабляет exact-spool provenance и является performance cleanup candidate, не correctness bug.

До реального benchmark не начинать database migration, persistent optimistic cache или arbitrary rotation threshold.

## CI / compile verification

Последняя исторически подтверждённая compile failure была missing closing brace в `CM_MaterialLedger.cpp`, исправленная:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8
```

Для текущего HEAD combined statuses/workflow runs через connector пока не видны. Это означает **CI NOT CONFIRMED**, а не GREEN.

Static diff review последних whole-file updates подтвердил:

- `CM_BackupExportWeb.cpp` changed only conductor whitelist entry;
- `CM_WindingJournal.cpp` changed only validator include + startup structural syntax check;
- `CM_WarehouseWeb.cpp` changed only service includes + route/service registration;
- `CM_MaterialLedger.cpp` changed только заявленные runtime/recovery validation paths.

Это не заменяет фактический ESP32 build.

## Следующее repo-only действие

1. Проверить фактический current-head ESP32 build/Actions, если run станет видимым.
2. Продолжать только по конкретно найденным correctness/compile gaps; не угадывать filenames и не создавать duplicate implementations.
3. Найти точную реализацию tri-state `WarehouseStore::repairExists(uint32_t, bool&)` только через подтверждённый branch path/history; проверить её fail-closed semantics. Не создавать второй symbol по предположению.
4. После hardware benchmark выбрать первый измеренный performance hotspot и только тогда проектировать bounded index/audit decomposition/rotation.

## Обязательный следующий внешний этап — hardware E2E

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

Снять один `/api/backup/manifest` после полного flow:

```text
items[].size_bytes
все 29 Stage 0 metrics
snapshot_stability_duration_ms
```

Fault cases отдельно:

- reboot/manual-review;
- microSD loss;
- corrupted persistence;
- UART timeout/reject/duplicate events;
- duplicate writeoff;
- writeoff wrong session/run/spool;
- close without wire coverage;
- backup during active winding.

Только после этого эксплуатационную готовность можно поднимать существенно выше 90%.
