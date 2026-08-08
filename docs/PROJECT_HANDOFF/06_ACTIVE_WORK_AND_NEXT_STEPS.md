# Где остановились и что делать дальше

Дата обновления: 2026-08-08
Ветка: `cmp-protocol-v1`

Самый полный свежий snapshot находится в:

```text
docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md
```

Этот файл фиксирует именно активную очередь работ.

## Что уже закрыто — не делать повторно

Основной production path уже собран:

```text
client
→ motor
→ repair OPEN
→ costing
→ linked winding
→ exact spool_id
→ immutable snapshot + spool-selection
→ UART delivery
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ winding history
→ manual wire writeoff
→ source_session_id + source_run_id
→ materials/pricing
→ finalization preflight
→ CLOSED
→ archive/report
→ read-only backup/export
```

Не начинать заново:

- persistent allocator;
- snapshot/state/recovery;
- exact spool selection;
- linked repair/motor validation;
- winding journal/history;
- repair CLOSED lifecycle;
- warehouse/material crash recovery;
- strict costing;
- run-level manual wire provenance;
- finalization wire coverage;
- reports;
- whitelist backup/export;
- deep backup persistence integrity audits.

## Последний CI факт

Пользователь передал Actions run:

```text
31243187630
```

`build-esp32` job:

```text
93067378338
```

Run собирал commit:

```text
78ac24533f1157080bd2163990dbdb0b2577807c
```

Первая реальная compile error:

```text
firmware/esp32/src/CM_MaterialLedger.cpp:705:1:
error: expected '}' at end of input
```

Причина: отсутствующая closing brace namespace `CM`. Framework `WString.h [-Wconversion]` были только warnings.

Исправлено:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8  Fix MaterialLedger namespace closure
```

Нельзя утверждать GREEN для `77fd7dd4`, пока фактический новый ESP32 Actions run не подтверждён.

## Текущий backup/export контракт

Глубокий `snapshot_stable` audit выполняется только когда `BackupActivityGuard` возвращает `Safe`.

Во время active winding:

```text
export_allowed=false
snapshot_stability_checked=false
snapshot_stable=null
```

Это важно: manifest не делает тяжёлые full scans microSD во время намотки.

Когда state safe, deep audit проверяет:

- material recovery markers и persisted files;
- workshop clients/motors/repairs/status;
- repair pricing + repair references;
- winding journal schema и transition semantics;
- warehouse spools/price/movements;
- allocator main/backup/temp state;
- conductor settings + temp marker;
- session snapshot/state/spool-selection contents и cross-identity.

Ключевая последняя серия коммитов перечислена полностью в `12_LATEST_HANDOFF_2026-08-08.md`.

## Следующее действие №1 — cleanup winding backup audit

Перечитать актуальные:

```text
firmware/esp32/src/CM_WindingPersistenceIntegrityAudit.cpp
firmware/esp32/src/CM_WindingPersistenceIntegrityAudit.h
firmware/esp32/src/CM_WindingJournalQuery.h
firmware/esp32/src/CM_WindingJournalQueryValidation.cpp
```

Цель:

перевести full-file schema validation внутри `CM_WindingPersistenceIntegrityAudit` на authoritative:

```text
WindingJournalQuery::validateAll()
```

вместо собственного pagination/cursor обхода.

Ограничения:

- не запускать write/recovery;
- сохранить read-only semantics;
- сохранить transition audit;
- не ослабить fail-closed behavior;
- deep audit всё ещё только при `BackupActivityGuard::Safe`.

## Следующее действие №2 — новый ESP32 Actions failure, если появится

Если пользователь даёт новый run URL/ID:

1. получить jobs run;
2. найти failed `build-esp32` job;
3. загрузить полный job log;
4. искать первую реальную `error:` / `fatal error:` / linker failure;
5. не диагностировать по хвосту framework warnings;
6. fetch exact current file из `cmp-protocol-v1`, исправить минимально и commit.

## Следующее действие №3 — HTTP/error semantics backup

Проверить новые endpoints/fields на единый контракт:

```text
bad request -> 400
active/unsafe export -> 409 или manifest blocked state
not found -> 404
integrity/read failure -> 500
storage unavailable -> 503
```

Особенно проверить:

- `snapshot_stability_checked`;
- nullable `snapshot_stable`;
- `snapshot_stability_reason`;
- `blocked_reason`;
- session directory malformed/temp/read failures;
- новые reasons:
  - `persistent_id_unstable_or_invalid`;
  - `conductor_settings_unstable_or_invalid`;
  - `business_data_unstable_or_invalid`;
  - `material_persistence_unstable_or_invalid`;
  - `winding_persistence_unstable_or_invalid`;
  - `warehouse_persistence_unstable_or_invalid`;
  - `warehouse_movements_unstable_or_invalid`;
  - `session_persistence_unstable_or_invalid`.

## Следующее действие №4 — performance/rotation review

После integrity review оценить стоимость растущих full scans:

- manifest deep audit;
- finalization preflight;
- costing;
- repair histories;
- warehouse/material NDJSON.

Не мигрировать преждевременно в БД. Сначала измерить/оценить реальные размеры и частоту. Возможные направления только при необходимости:

- bounded indexes;
- summary snapshots;
- rotation/archiving старых immutable logs;
- cache только там, где corruption всё равно проверяется fail-closed перед mutation.

## Hardware E2E — обязательный внешний этап

Repository review и CI не доказывают физическое поведение ESP32 + Arduino.

Happy path:

```text
1. client + motor + repair
2. warehouse price + active CU/AL spool
3. linked winding + exact spool_id
4. JOB_ACK ACCEPTED
5. physical START
6. RUN_STARTED
7. RUN_COMPLETED
8. winding history + immutable spool identity
9. manual wire writeoff
10. source_session_id + source_run_id
11. costing
12. finalization preflight
13. CLOSED
14. archive + monthly report
15. stable backup manifest/export
```

Fault scenarios:

- microSD unavailable before job;
- runtime microSD loss;
- reboot after ACCEPTED before START;
- reboot after RUN_STARTED;
- corrupted snapshot/state/spool-selection/journal;
- corrupted warehouse/material/pricing/workshop data;
- dangling warehouse PENDING;
- material pending/swap marker;
- duplicate writeoff same `(session_id, run_id)`;
- close without manual wire coverage;
- backup during active winding;
- backup with temp/pending/corrupt state.

## Deferred — не начинать автоматически

- analogue/unassigned winding production mode;
- automatic writeoff только по `RUN_COMPLETED`;
- automatic safe resume;
- database migration;
- direct SSR control с ESP32/WEB.

## Правило переноса

Новый чат читает в таком порядке:

```text
00_READ_FIRST.md
12_LATEST_HANDOFF_2026-08-08.md
01_CURRENT_STATE.md
06_ACTIVE_WORK_AND_NEXT_STEPS.md
актуальные исходники
09_KEY_FILES_INDEX.md
08_WORK_RULES_AND_VERIFICATION.md
```

Код `cmp-protocol-v1` всегда выше документации по приоритету.
