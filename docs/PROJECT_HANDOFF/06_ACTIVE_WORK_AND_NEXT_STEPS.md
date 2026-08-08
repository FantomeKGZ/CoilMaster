# Где остановились и что делать дальше

Дата обновления: 2026-08-08
Ветка: `cmp-protocol-v1`
Ориентировочная функциональная готовность: около 90%

## Точная точка продолжения

Основной production path замкнут:

```text
клиент
→ двигатель
→ ремонт OPEN
→ калькуляция
→ linked winding
→ обязательный exact spool_id
→ immutable job snapshot + immutable spool selection
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ winding history
→ ручное списание фактического расхода провода
→ source_session_id + source_run_id provenance
→ дополнительные материалы / pricing
→ read-only finalization preflight
→ CLOSED
→ read-only архив
→ месячный отчёт
→ read-only backup/export
```

Физический START, SSR и автоматическое продолжение после reboot не управляются WEB/ESP32. `RUN_COMPLETED` сам провод не списывает.

## Exact spool + ручное списание

Новые linked jobs требуют конкретную активную CU/AL бухту с положительным остатком. Выбор сохраняется отдельно как immutable spool-selection до UART delivery.

Manual write-off provenance имеет гранулярность:

```text
source_session_id + source_run_id
```

Backend доказывает:

- конкретный `RUN_COMPLETED(session, run)` существует;
- immutable spool-selection принадлежит тому же repair/motor;
- write-off использует тот же `spool_id`;
- для этого `(session_id, run_id)` ещё нет CONFIRMED write-off;
- `PENDING → CONFIRMED | ABORTED` сохраняет provenance через reboot recovery.

Repair finalization требует ручное покрытие каждого нового completed linked run с immutable spool-selection. Legacy sessions без spool-selection остаются совместимыми.

## Backup/export

Доступны read-only whitelist endpoints:

```text
GET /api/backup/manifest
GET /api/backup/file?name=...
GET /api/backup/sessions
GET /api/backup/session-file?kind=...&session_id=...
```

Arbitrary filesystem paths запрещены. Тяжёлый export блокируется во время активной winding phase.

Manifest разделяет:

- `export_allowed` — безопасно ли сейчас выполнять тяжёлый export;
- `snapshot_stable` — доказана ли целостность persistent snapshot.

Нестабильная карта всё ещё может быть выгружена как диагностическая копия, если export разрешён, но UI не должен называть её чистым backup.

### Что теперь входит в `snapshot_stable`

Read-only audit проверяет:

- material recovery markers (`usage.pending`, `adjustment.pending`, material swap temp/backup);
- warehouse spool swap temp/backup;
- `materials.ndjson`, `usage.ndjson`, `adjustments.ndjson` и их арифметику;
- references material usage → repair/material и adjustment → material;
- workshop clients/motors/repairs + repair-status через authoritative registry validation;
- `repair-pricing` structure и pricing → repair references;
- warehouse `spools`, `price`, `movements` transaction integrity и movement → spool/repair references;
- persistent allocator `id-state.txt`, optional `id-state.bak`, отсутствие `id-state.tmp`;
- winding journal schema до EOF;
- winding transition state-machine;
- session directories snapshots/state/spool-selection;
- содержимое каждого session file штатными parsers;
- cross-file `job_id/session_id/repair_id/motor_id` identity.

Read-only правило важно: session directories полностью проверяются на canonical `session-N.json` до запуска store parser. GET backup audit не выполняет recovery/rename.

Legacy snapshot-only session допустима. Runtime state без snapshot недопустим. Spool-selection требует snapshot + state и совпадающую linked identity.

## Последний backup-integrity блок

```text
8004210a  Add material persistence integrity audit contract
f0f9c9e1  Implement material persistence integrity audit
b8e368ef  Audit material persistence in backup manifest
3f72d3f0  Add workshop persistence integrity audit contract
dcabd2c7  Implement workshop persistence integrity audit
2efe0f0b  Add repair pricing integrity audit contract
3ccec83a  Implement repair pricing integrity audit
4328e5b9  Extend backup persistence integrity audit
15ec0d86  Add full winding journal validation contract
42389e54  Implement full winding journal validation
40b5132c  Audit winding persistence in backup stability
97d63e41  Add warehouse persistence integrity audit contract
917e16fb  Implement warehouse persistence integrity audit
9553c6ff  Audit warehouse persistence in backup stability
c6284595  Add persistent ID integrity audit contract
f564bcd1  Implement persistent ID integrity audit
423f0bda  Audit persistent allocator state in backup stability
a0edb0eb  Add winding session persistence audit contract
34f2666c  Implement winding session persistence audit
9a2f024f  Audit winding session metadata in backup stability
bbec520c  Keep winding session backup audit strictly read only
a18193ee  Validate pricing repair references in backup audit
dc569bb0  Validate material persistence references in backup audit
50c7da18  Validate warehouse movement references in backup audit
0b3b7b4f  Preserve legacy snapshot only session compatibility
```

## Уже закрытые архитектурные задачи — не делать повторно

- persistent `job_id/session_id` allocator;
- immutable job snapshot;
- immutable exact spool selection;
- persistent runtime-state;
- fail-safe recovery/manual review;
- strict repair/motor linkage;
- authoritative `coil_program`;
- winding journal schema 2 + strict history;
- semantic winding transition audit;
- dynamic microSD readiness;
- append-only repair `OPEN → CLOSED`;
- CLOSED mutation guards;
- warehouse crash recovery;
- material usage/adjustment crash recovery;
- strict warehouse/material/costing parsing;
- repair finalization preflight;
- closed-repair reporting;
- run-level manual wire writeoff provenance;
- duplicate provenance prevention;
- manual wire coverage requirement before CLOSED;
- whitelist read-only backup/export;
- backup activity guard;
- full backup persistence integrity audit.

## Следующее обязательное действие — hardware E2E

Repository review и CI не доказывают физическое поведение ESP32 + Arduino.

Минимальный happy path:

```text
1. клиент + двигатель + ремонт
2. warehouse price + активная CU/AL бухта
3. linked winding + exact spool_id
4. JOB_ACK ACCEPTED
5. physical START
6. RUN_STARTED
7. RUN_COMPLETED
8. проверить winding history + immutable spool identity
9. вручную подтвердить фактический wire write-off
10. проверить source_session_id + source_run_id
11. проверить costing
12. finalization preflight
13. CLOSED
14. архив + месячный отчёт
15. стабильный backup manifest/export
```

Обязательные fault/recovery сценарии:

- microSD unavailable до job;
- microSD loss после boot;
- reboot после ACCEPTED до START;
- reboot после RUN_STARTED;
- corrupted snapshot/state/spool-selection/journal;
- corrupted warehouse/material/pricing/workshop data;
- dangling warehouse PENDING;
- material pending/swap marker;
- повторный write-off одного `(session_id, run_id)`;
- close без manual wire coverage;
- backup во время active winding;
- backup при temp/pending/corrupt persisted state.

## Следующий repo-reviewable кодовый приоритет

Если стенд временно недоступен:

1. привести operator-facing backup reason к нейтральному имени после расширения audit scope (`material_persistence_unstable_or_invalid` сейчас исторически слишком узкое имя);
2. короткий audit HTTP/error semantics по новым backup/run-level кодам;
3. performance/rotation план для растущих NDJSON без преждевременной миграции БД;
4. `analogue / unassigned winding` проектировать только как отдельное продуктовое решение, если он реально нужен мастерской, не ослабляя `repair ↔ motor ↔ coil_program`.

## Что намеренно не делать

- auto physical START;
- auto resume после reboot;
- direct SSR control с ESP32/WEB;
- automatic wire writeoff только по `RUN_COMPLETED`;
- arbitrary-path backup API;
- миграцию NDJSON в новую БД без фактической необходимости.

## Правило переноса

Новый чат сначала читает `00_READ_FIRST.md`, `01_CURRENT_STATE.md`, этот файл и актуальные исходники. Код `cmp-protocol-v1` всегда source of truth.
