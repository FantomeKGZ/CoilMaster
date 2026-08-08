# Где остановились и что делать дальше

Дата обновления: 2026-08-08  
Ветка: `cmp-protocol-v1`

Код ветки всегда выше документации по приоритету. Полный snapshot: `docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md`.

## Текущая оценка готовности

Ориентировочно:

- функциональный production flow: **около 94%**;
- repository/firmware readiness: **около 90%**;
- общая эксплуатационная готовность: **около 89%**.

Последний показатель ниже из-за отсутствия подтверждённого GREEN ESP32 build текущего HEAD и обязательного полного hardware E2E ESP32 + Arduino.

## Уже закрыто — не повторять

Production flow собран:

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual wire writeoff
→ source_session_id + source_run_id → materials/pricing
→ finalization preflight → CLOSED → archive/report → read-only backup
```

Также закрыты:

- persistent allocator/session state/recovery;
- strict repair/motor/coil_program linkage;
- exact spool selection и run-level provenance;
- winding history + transition validation;
- warehouse/material recovery и costing;
- finalization coverage;
- whitelist backup/export + deep persistence integrity;
- backup winding scan через `WindingJournalQuery::validateAll()` + transition audit;
- backup/run-level HTTP semantics audit;
- Stage 0 backup observability до 29 metrics;
- flat persisted JSON hardening основных backup/runtime/history/writeoff readers.

Safety invariants не менять:

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- после reboot нет auto-resume;
- `RUN_COMPLETED` сам по себе не списывает wire;
- wire writeoff остаётся ручным и связан с exact `spool_id`, `source_session_id + source_run_id`.

## Stage 0 performance observability

Manifest возвращает **29 metrics без telemetry-only full scan**.

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

Не добавлять новые metrics ради количества. До benchmark не вводить arbitrary rotation threshold, persistent optimistic cache или database migration.

## Flat persisted JSON correctness hardening

Shared validator:

```text
firmware/esp32/src/CM_FlatJsonObjectValidator.h
```

Он проверяет полный синтаксис уже прочитанного flat JSON object без внешней JSON dependency и без нового filesystem pass.

Уже hardened authoritative/runtime readers включают:

```text
CM_BackupBusinessDataIntegrityAudit
CM_RepairRegistry
CM_RepairPricingIntegrityAudit
CM_MaterialPersistenceIntegrityAudit
CM_WarehousePersistenceIntegrityAudit
CM_WarehouseMovementIntegrityAudit
CM_ConductorSettingsIntegrityAudit
CM_RepairCosting
CM_MaterialHistory
CM_MaterialUsageHistory
CM_MaterialAdjustment
CM_MaterialLedgerCurrency
CM_RepairPricingHistory
CM_WarehouseWriteOffHistory
CM_WarehouseStore
CM_WarehouseWriteOff
CM_WarehousePrice
CM_WarehouseSpoolIdentity
CM_RepairLifecycle
CM_WindingJournalTransitionAudit
CM_WindingJournalQuery
```

Repeated O(n²)/O(n*m) identity/reference scans не должны получать parser multiplier, если соответствующий authoritative outer pass уже доказал syntax.

## Последний winding/finalization safety block

### Repair lifecycle

`CM_RepairLifecycle.h` теперь использует `FlatJsonObjectValidator` для `/data/workshop/repair-status.ndjson`.

Commit:

```text
fcae62f726ea82d3dfecb2a432d969e683e069ab  Fail closed on malformed repair lifecycle state
```

Это влияет на OPEN/CLOSED preflight, используемый costing/material/writeoff paths.

### Finalization journal validation

`CM_RepairFinalizationGuard.cpp` больше не делает полный integrity scan через cursor-pagination `appendHistoryJson()` с временными 4 KB JSON pages.

Теперь используется:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

Commit:

```text
d92b6f1c19a5d0c5c2abb3663410e9202bcd385e  Use authoritative winding validation for finalization
```

Тот же guard используется и read-only finalization preview, и фактическим `POST /api/repairs/close`, поэтому CLOSED invariant сохранён.

### Manual wire writeoff completion proof

`CM_WindingSessionCompletionAudit.cpp` раньше доказывал в основном наличие `RUN_COMPLETED` для session/run.

Теперь перед lookup конкретного run выполняются:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

То есть ручное списание не разрешается на основе изолированной `RUN_COMPLETED`, если журнал имеет schema corruption или нарушенную последовательность `RUN_STARTED → RUN_COMPLETED`.

Commit:

```text
9e18cd076b8e1ad904e3893aab20cde1a4e596a8  Require authoritative winding integrity before writeoff
```

Это дополнительный read на ручной safety-critical операции; он введён по correctness-причине, а не как performance feature.

### Transition audit JSON syntax

`CM_WindingJournalTransitionAudit.cpp` теперь сам fail-closed проверяет flat JSON syntax перед state-machine parsing.

Commit:

```text
6a9d47f889ef0197fcb5b85b3c838984d3fb4e74  Reject malformed JSON in winding transition audit
```

### `validateAll()` теперь действительно strict JSON authority

Repository review выявил, что `WindingJournalQuery::validateAll()` вызывал schema1/schema2 validators, которые ранее проверяли внешние `{...}` и required fields, но не полный JSON syntax.

`CM_WindingJournalQuery.cpp` теперь вызывает `FlatJsonObjectValidator::valid(line)` внутри обоих record validators.

Commit:

```text
f3e02dadce34c7a85e11d23154652a469d6aa111  Make winding validateAll reject malformed JSON
```

Практически это усиливает сразу:

- deep backup winding integrity;
- finalization/CLOSED preflight;
- manual writeoff completion proof;
- cursor history API parsing.

## Явно незакрытые repo-only correctness кандидаты

### 1. `CM_MaterialLedger.cpp`

В большом файле остаются direct persisted-row paths со старым selected-field parsing:

- material catalog JSON output;
- material row scan перед usage confirmation;
- pending usage recovery/durable lookup;
- direct stock lookup;
- generic next ID scan;
- quantity rewrite/restore source rows.

Обычный большой `update_file` ранее был остановлен connector safety-filter **до записи**. Не использовать low-level Git object workflow для обхода filter. Если обычный SHA-guarded update проходит — harden rows shared validator’ом без broad audit перед каждой mutation.

### 2. Startup `WindingJournal::begin()`

`main.cpp` на boot делает:

```text
journalReady = sdReady && journal.begin();
```

`CM_WindingJournal.cpp::validateJournalStructure()` всё ещё использует старый outer-brace/selected-field parsing. После изменения `WindingJournalQuery` backup/finalization/writeoff authority strict, но startup journal readiness ещё не полностью выровнена с новым flat-JSON contract.

Перед изменением большого `CM_WindingJournal.cpp` выбрать аккуратный granular approach. Не добавлять `FlatJsonObjectValidator` во все повторные helper scans вслепую: event save path уже выполняет несколько full scans, поэтому parser multiplier надо оценивать отдельно.

Приоритет: сделать startup authoritative syntax proof с минимальным дополнительным I/O/CPU и сохранить существующую transition/session-context semantics.

## Performance hotspots — пока измерять, не рефакторить

Подтверждены:

- `MaterialPersistenceIntegrityAudit::check()` транзитивно вызывает broad workshop/pricing dependency audits;
- business uniqueness/reference lookups имеют повторные scans;
- warehouse reference validation повторно ищет spool/repair references;
- backup preliminary session-directory scan идёт перед authoritative deep session audit;
- `WireWriteOffCoverageAudit` использует cursor pages журнала и повторный movements scan для completed runs.

Последний пункт может быть дорогим O(n*m), но сейчас он correctness-correct и закрывает coverage. Не заменять его новым index до фактического benchmark.

## CI status

Последняя ранее подтверждённая Actions failure была missing closing brace в `CM_MaterialLedger.cpp`, исправленная:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8
```

Новые commits не считать GREEN без фактического build result. Отсутствие status/workflow run в connector не является GREEN.

## Следующее практическое действие

Repo-only до стенда:

1. Попытаться закрыть `CM_MaterialLedger.cpp` только обычным SHA-guarded contents update.
2. Спроектировать минимальный startup strict-validation change для `WindingJournal::begin()` без parser multiplier по всем helper scans.
3. После каждого изменения — static compile/include review и фактический CI result, если connector его показывает.
4. Не начинать Stage 1 performance refactor до benchmark без отдельной correctness-причины.

Внешне обязательный этап — реальный hardware E2E ESP32 + Arduino:

```text
linked repair → exact spool → JOB_ACK → physical START
→ RUN_STARTED → RUN_COMPLETED → manual wire writeoff
→ costing → finalization preflight → CLOSED → stable backup
```

На стенде сохранить один `/api/backup/manifest`:

```text
items[].size_bytes
все 29 Stage 0 metrics
snapshot_stability_duration_ms
```

И отдельно fault cases:

- reboot/manual-review;
- microSD loss;
- corrupted persistence;
- UART faults;
- duplicate writeoff;
- close without wire coverage;
- backup during active winding.
