# Где остановились и что делать дальше

Дата обновления: 2026-08-08  
Ветка: `cmp-protocol-v1`

Код ветки всегда выше документации по приоритету. Полный snapshot: `docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md`.

## Уже закрыто — не повторять

Production flow собран:

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual wire writeoff
→ source_session_id + source_run_id → materials/pricing
→ finalization preflight → CLOSED → archive/report → read-only backup
```

Также уже закрыты:

- persistent allocator/session state/recovery;
- strict repair/motor/coil_program linkage;
- exact spool selection и run-level provenance;
- winding history + transition validation;
- warehouse/material recovery и costing;
- finalization coverage;
- whitelist backup/export + deep persistence integrity;
- winding `validateAll()` cleanup;
- backup/run-level HTTP semantics audit;
- Stage 0 backup observability до 29 metrics;
- flat persisted JSON syntax hardening для deep audits и основных runtime/history/writeoff readers.

## CI status

Последняя ранее подтверждённая ошибка Actions была missing closing brace в `CM_MaterialLedger.cpp`; fix:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8  Fix MaterialLedger namespace closure
```

Новые commits не считать GREEN без фактического build result. Для текущего head GREEN CI пока не подтверждён.

## Deep backup contract

Deep audit выполняется только при `BackupActivityGuard::Safe`. При active winding тяжёлый scan не запускается, export блокируется, stability/observability metrics остаются `null`.

Safe `snapshot_stable=true` означает проверку static whitelist, recovery markers, allocator/settings, workshop/pricing/material/warehouse references, winding schema/transitions и содержимого всех snapshot/state/spool-selection session files.

## Stage 0 performance observability

Manifest возвращает **29 metrics без отдельного telemetry scan**.

Per-domain timing:

```text
snapshot_stability_duration_ms
persistent_id_audit_duration_ms
conductor_settings_audit_duration_ms
material_persistence_audit_duration_ms
business_data_audit_duration_ms
winding_persistence_audit_duration_ms
warehouse_persistence_audit_duration_ms
warehouse_movements_audit_duration_ms
winding_session_directory_scan_duration_ms
winding_session_persistence_audit_duration_ms
```

Population/high-water/size:

```text
winding_allocator_last_id
material_catalog_record_count
material_usage_record_count
material_adjustment_record_count
workshop_client_record_count
workshop_motor_record_count
workshop_repair_record_count
repair_status_record_count
repair_pricing_record_count
winding_journal_record_count
winding_snapshot_file_count
winding_state_file_count
winding_spool_selection_file_count
winding_snapshot_total_bytes
winding_state_total_bytes
winding_spool_selection_total_bytes
warehouse_spool_record_count
warehouse_price_record_count
warehouse_movement_record_count
```

Timing оборачивает только уже существующие audit calls через `millis()`; filesystem I/O ради duration не добавляется. Если audit дошёл до domain, его duration публикуется даже при failure этого domain, а последующие неисполненные domains остаются `null`.

`winding_session_directory_scan_duration_ms` отдельно измеряет preliminary directory scan, а `winding_session_persistence_audit_duration_ms` — authoritative deep parser/cross-identity audit. До benchmark эти passes не объединять.

Record counts, allocator high-water и session file counts/bytes собираются внутри уже выполняемых authoritative passes. Старые `check(storage)` contracts сохранены совместимо. Partial domain counts/high-water после failed domain audit наружу не публикуются.

Session byte totals являются telemetry-only: при 32-bit aggregate overflow только total-byte fields становятся `null`; integrity result не меняется.

Ключевой timing commit:

```text
96a1c5bc8c4a5cb7f5b672d290bbac23867429c5  Measure deep backup domain durations
```

## Flat persisted JSON correctness hardening

Repository review подтвердил correctness-причину: несколько persisted flat-NDJSON readers проверяли только внешние `{...}` и выбранные поля. Синтаксически повреждённая строка с ещё читаемым ID могла пройти часть integrity/startup/runtime checks или попасть в JSON API/агрегаты.

Добавлен общий header-only:

```text
firmware/esp32/src/CM_FlatJsonObjectValidator.h
```

Он проверяет полный синтаксис уже прочитанного flat JSON object без нового SD pass и без внешней JSON dependency.

### Deep/startup authoritative readers

Hardened:

```text
CM_BackupBusinessDataIntegrityAudit
CM_RepairRegistry
CM_RepairPricingIntegrityAudit
CM_MaterialPersistenceIntegrityAudit
CM_WarehousePersistenceIntegrityAudit
CM_WarehouseMovementIntegrityAudit
CM_ConductorSettingsIntegrityAudit
```

Strict parser выполняется один раз на authoritative outer pass. В известных O(n²)/O(n*m) duplicate/reference scans повторный full JSON parse намеренно не выполняется.

### Runtime/history/costing readers

Дополнительно hardened:

```text
CM_RepairCosting.cpp
CM_MaterialHistory.cpp
CM_MaterialUsageHistory.cpp
CM_MaterialAdjustment.cpp
CM_MaterialLedgerCurrency.cpp
CM_RepairPricingHistory.cpp
CM_WarehouseWriteOffHistory.cpp
CM_WarehouseStore.cpp
CM_WarehouseWriteOff.cpp
CM_WarehousePrice.cpp
CM_WarehouseSpoolIdentity.cpp
```

Практический эффект:

- malformed persisted history не должен попадать в material/pricing/write-off JSON response;
- costing не строит operator-visible totals поверх malformed movement/usage/pricing rows;
- material adjustment/recovery блокируется до temp swap/replay при malformed source/pending/audit JSON;
- material currency lookup fail-closed;
- warehouse summary и ID allocation fail-closed;
- manual writeoff проверяет movement/spool state до PENDING/atomic spool rewrite;
- rewritten spool/material-adjustment rows дополнительно syntax-проверяются до temp-file write;
- exact active spool identity и warehouse price fail-closed проверяются непосредственно в runtime preflight.

Safety invariants не менялись: physical START только физический, SSR остаётся у Arduino, auto-resume отсутствует, `RUN_COMPLETED` не списывает wire автоматически, writeoff остаётся ручным и связан с exact spool/session/run provenance.

Ключевые базовые commits:

```text
9ddabf613f1edf95dc1da55cbba8763414e47968  Add flat persisted JSON syntax validator
96c863b1a1bde3a3725596940e74805da2c69111  Reject malformed flat JSON in business backup audit
d13269bb481d056623569b9ecdf91708be6b0b8b  Fail closed on malformed workshop JSON
07b20e9b88012446fcbc813e78b39349ecd70753  Reject malformed flat JSON in pricing audit
b86794238cab420d1d97c3b281fa233b92ccf317  Avoid repeated JSON parsing in business identity scans
86b19f35cad6c383377dee9c342e58f4978b0e79  Avoid repeated JSON parsing in registry duplicate scans
16f39b33cccaeed533c39c2e0144d6942169b2c7  Reject malformed flat JSON in material persistence audit
b3fd050c5e917691877e1c245fadde333742eed7  Reject malformed flat JSON in warehouse persistence audit
b7b362bfe1813f27eab0c904dc9c7fc4489e6f9e  Reject malformed flat JSON in movement audit
ab0b1f6b0381641e811fed5a18ac412ebb0667d2  Reject malformed flat JSON in conductor settings audit
090acf40fc4470c7b8719975df7f4ce218a3cdec  Keep pricing reference scans identity focused
02a80ea88a157ebfaee389d9c87368b36235ccbd  Fail closed on malformed costing histories
```

Ключевые последующие runtime commits:

```text
4d1761aa7b2b958a880b7de2b30c3bc8b09e62cd  Reject malformed pricing history JSON
9fb8918eb8fad0c939cf04330ddfce81073c1f8c  Reject malformed write-off history JSON
b43568c4231d9c972ed92f0b3b23c038837e3252  Reject malformed material usage history JSON
8c3f2b76464918a889a0e5a42d27c6ac4ff82c63  Fail closed on malformed warehouse summary state
d5c0a99449132fd343e1dc04e92e4136471dfbbb  Fail closed on malformed write-off transaction state
ea37009b15cf5367d4c5712bdd412c16f4649827  Fail closed on malformed warehouse price state
e01b7f8e6219558622a1c86c517c4d200f767992  Fail closed on malformed spool identity state
```

Дополнительные material runtime commits находятся в текущей ветке для `CM_MaterialHistory.cpp`, `CM_MaterialAdjustment.cpp` и `CM_MaterialLedgerCurrency.cpp`; точная запись сессии находится в `10_SESSION_LOG.md`.

## Явно незакрытый repo-only кандидат

`firmware/esp32/src/CM_MaterialLedger.cpp` всё ещё содержит несколько direct persisted-row paths со старым shape shortcut/выборочным parsing:

- material catalog JSON output;
- material row scan перед usage confirmation;
- pending usage recovery/durable lookup;
- direct stock lookup;
- generic next ID scan;
- quantity rewrite/restore source rows.

Файл был fetched и подготовлен к аналогичному hardening, но большой GitHub `update_file` был остановлен connector safety-filter **до записи**. Low-level Git object workflow для обхода этого filter намеренно не использовался. Поэтому `CM_MaterialLedger.cpp` не считать исправленным.

Если connector позже позволяет обычный SHA-guarded contents update — это следующий конкретный repo-only correctness fix. Не заменять его broad full-audit вызовом перед каждой mutation: это добавило бы лишний SD I/O и исказило performance contract.

## Repo-reviewable integration status

Подтверждено на уровне static repository review:

- winding `validateAll()` + transition audit сохранены;
- session authoritative deep parser/cross-file identity audit не дублирован;
- compatibility audit overloads сохранены;
- Stage 0 telemetry не добавляет full scans;
- shared flat-JSON validator self-contained через `Arduino.h`;
- runtime/deep authoritative rows fail-closed по синтаксису в перечисленных modules;
- repeated identity/reference scans не получили лишний parser multiplier;
- `BackupActivityGuard::Safe` gating сохранён;
- ручной writeoff transaction/provenance semantics не изменены.

Это static repository review, а не доказательство успешной ESP32 сборки или hardware behavior.

## Подтверждённые performance hotspots для измерения

Repository review показывает:

- `MaterialPersistenceIntegrityAudit::check()` транзитивно вызывает broad `WorkshopPersistenceIntegrityAudit::check()` + pricing audit, а backup затем проверяет часть domains снова;
- `BackupBusinessDataIntegrityAudit` использует повторные uniqueness/reference lookups;
- warehouse reference validation повторно ищет spool/repair references;
- backup делает preliminary session-directory scan до authoritative deep session audit.

Это не correctness bugs. Per-domain durations позволяют сначала измерить цену каждого пути.

## Следующее практическое действие

Внешне обязательный этап — реальный hardware E2E ESP32 + Arduino и benchmark одного backup manifest:

```text
items[].size_bytes
все 29 Stage 0 metrics
snapshot_stability_duration_ms
```

Сначала выбрать самый дорогой domain по `*_duration_ms`, затем объяснить его рост через record counts/session bytes/high-water. Только после этого решать bounded in-request index, duplicate-audit decomposition или rotation.

Repo-only до стенда:

1. `CM_MaterialLedger.cpp` — только если обычный SHA-guarded update проходит connector safety filter.
2. Дальше только фактически найденные runtime/recovery readers с подтверждённой correctness-причиной; не угадывать пути и не добавлять metrics ради количества.
3. Не начинать Stage 1 duplicate-scan refactor, rotation threshold, persistent optimistic cache или database migration до benchmark без отдельной correctness-причины.

## Hardware E2E — обязательно отдельно

Repository review и CI не доказывают физический ESP32 + Arduino path. Проверить:

```text
linked repair → exact spool → JOB_ACK → physical START
→ RUN_STARTED → RUN_COMPLETED → manual wire writeoff
→ costing → finalization preflight → CLOSED → stable backup
```

И fault cases: reboot/manual-review, microSD loss, corrupted persistence, UART faults, duplicate writeoff, close without wire coverage, backup during active winding.

Safety invariants не менять: no automatic physical START, no auto-resume, no direct ESP32/web SSR control, no automatic wire writeoff on `RUN_COMPLETED`, corruption/storage loss fail closed.
