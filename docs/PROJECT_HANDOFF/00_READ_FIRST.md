# CoilMaster — продолжение проекта

Дата обновления: 2026-08-08

Репозиторий: `FantomeKGZ/CoilMaster`  
Рабочая ветка: `cmp-protocol-v1`

Этот каталог хранит контекст для переноса разработки в новый чат.

## Запрос для нового чата

> Открой репозиторий `FantomeKGZ/CoilMaster`, работай только с веткой `cmp-protocol-v1`. Сначала прочитай `docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md`, затем `01_CURRENT_STATE.md`, `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и `09_KEY_FILES_INDEX.md`. После этого обязательно заново fetch актуальные версии исходников, которые собираешься менять. Код `cmp-protocol-v1` всегда source of truth. Продолжи с точной точки из `12_LATEST_HANDOFF_2026-08-08.md` и после значимого этапа снова обнови handoff.

## Рекомендуемый порядок чтения

1. `12_LATEST_HANDOFF_2026-08-08.md` — самый свежий полный snapshot: сделанное, последний CI failure/fix и точный следующий план.
2. `01_CURRENT_STATE.md` — архитектурное текущее состояние.
3. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — активные задачи и hardware E2E.
4. `09_KEY_FILES_INDEX.md` — ключевые файлы и пути.
5. `08_WORK_RULES_AND_VERIFICATION.md` — правила работы и проверки.
6. `02_ARCHITECTURE_AND_HARDWARE.md` — аппаратная архитектура.
7. `03_PROTOCOL_AND_WINDING_FLOW.md` — CMP/UART и winding flow.
8. `04_DATA_STORAGE_API_UI.md` — данные/API/UI; сверять с кодом.
9. `05_COMPLETED_WORK_LOG.md` — укрупнённая история реализованного.
10. `10_SESSION_LOG.md` — журнал рабочих сессий.
11. `07_BACKLOG_AND_DEFERRED.md` — отложенное; не считать реализованным автоматически.
12. `11_FULL_BRANCH_AUDIT.md` — историческая карта, не источник текущего кода.

## Источник истины

Приоритет:

1. текущий код ветки `cmp-protocol-v1`;
2. фактические результаты актуальных Actions/build/tests;
3. `12_LATEST_HANDOFF_2026-08-08.md`;
4. `01_CURRENT_STATE.md` + `06_ACTIVE_WORK_AND_NEXT_STEPS.md`;
5. остальные handoff-файлы;
6. тематические документы `docs/*`;
7. история чатов.

Перед изменением существующего файла всегда заново получать его текущее содержимое и blob SHA из `cmp-protocol-v1`. Для нового файла сначала проверять отсутствие точного пути.

## Критические safety-правила

- Не переключаться на `main` как источник реализации.
- Не начинать заново persistent allocator, snapshots/state/recovery, linked-job, winding history, repair lifecycle, warehouse/material/costing, exact spool provenance или backup — они уже реализованы.
- ESP32/WEB не должны напрямую управлять SSR.
- Физический START остаётся обязательным.
- Automatic resume/automatic queue после recovery запрещены.
- `RUN_COMPLETED` не должен автоматически списывать провод.
- Не ослаблять fail-closed semantics ради UI convenience.

## Последний CI факт

Actions run `31243187630` на commit `78ac245...` падал не из-за `WString.h` warnings, а из-за:

```text
CM_MaterialLedger.cpp: expected '}' at end of input
```

Исправление уже закоммичено:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8  Fix MaterialLedger namespace closure
```

Не считать этот fix доказанно GREEN, пока нет фактического успешного ESP32 Actions run после него.

## Текущая точка продолжения

Deep backup-integrity, winding `validateAll()` cleanup и backup/run-level HTTP semantics audit уже завершены.

Stage 0 performance observability теперь даёт без дополнительного persistence scan:

```text
snapshot_stability_duration_ms
winding_journal_record_count
warehouse_movement_record_count
```

На hardware E2E/эксплуатационном стенде нужно сопоставить их с `winding-events.size_bytes` и `warehouse-movements.size_bytes`. До измерений не вводить rotation threshold, persistent cache или database migration.

Если hardware пока недоступен, следующий repo-only шаг допустим только как такой же same-pass observability для уже выполняемого authoritative validator; естественный кандидат — material usage/adjustment audit, без второго full scan ради метрики.

Все детали и список последних коммитов: `12_LATEST_HANDOFF_2026-08-08.md`.
