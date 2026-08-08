# CoilMaster — продолжение проекта

Дата обновления: 2026-08-08 22:22 +06

Репозиторий: `FantomeKGZ/CoilMaster`  
Рабочая ветка: `cmp-protocol-v1`

Этот каталог хранит контекст для переноса разработки в новый чат.

## Запрос для нового чата

> Открой репозиторий `FantomeKGZ/CoilMaster`, работай только с веткой `cmp-protocol-v1`. Сначала прочитай `docs/PROJECT_HANDOFF/13_PAUSE_HANDOFF_2026-08-08_2222.md`, затем `06_ACTIVE_WORK_AND_NEXT_STEPS.md`, `01_CURRENT_STATE.md`, `09_KEY_FILES_INDEX.md` и при необходимости `12_LATEST_HANDOFF_2026-08-08.md`. После этого обязательно заново fetch актуальные версии исходников, которые собираешься менять. Код `cmp-protocol-v1` всегда source of truth. Перед каждым update использовать текущий blob SHA; при `409` re-fetch + merge. Не использовать `main` как источник реализации.

## Рекомендуемый порядок чтения

1. `13_PAUSE_HANDOFF_2026-08-08_2222.md` — самая свежая точка паузы и точное продолжение.
2. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — текущие активные задачи и hardware E2E.
3. `01_CURRENT_STATE.md` — архитектурное состояние.
4. `09_KEY_FILES_INDEX.md` — ключевые файлы и пути.
5. `12_LATEST_HANDOFF_2026-08-08.md` — предыдущий полный snapshot.
6. `08_WORK_RULES_AND_VERIFICATION.md` — правила работы и проверки.
7. `02_ARCHITECTURE_AND_HARDWARE.md` — аппаратная архитектура.
8. `03_PROTOCOL_AND_WINDING_FLOW.md` — CMP/UART и winding flow.
9. `04_DATA_STORAGE_API_UI.md` — данные/API/UI; сверять с текущим кодом.
10. `05_COMPLETED_WORK_LOG.md` — укрупнённая история реализованного.
11. `10_SESSION_LOG.md` — подробный журнал рабочих сессий.
12. `07_BACKLOG_AND_DEFERRED.md` — отложенное; не считать реализованным автоматически.
13. `11_FULL_BRANCH_AUDIT.md` — историческая карта, не источник текущего кода.

## Источник истины

Приоритет:

1. текущий код ветки `cmp-protocol-v1`;
2. фактические результаты актуальных Actions/build/tests;
3. `13_PAUSE_HANDOFF_2026-08-08_2222.md`;
4. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` + `01_CURRENT_STATE.md`;
5. `12_LATEST_HANDOFF_2026-08-08.md`;
6. остальные handoff-файлы;
7. тематические документы `docs/*`;
8. история чатов.

Перед изменением существующего файла всегда заново получать его текущее содержимое и blob SHA из `cmp-protocol-v1`. Для нового файла сначала проверять отсутствие точного пути.

## Критические safety-правила

- ESP32/WEB не управляют SSR напрямую.
- Физический START остаётся обязательным.
- Automatic resume после reboot запрещён.
- `RUN_COMPLETED` не выполняет automatic wire writeoff.
- Новый wire writeoff остаётся ручным и требует exact `spool_id + source_session_id + source_run_id`.
- Не ослаблять fail-closed semantics ради UI convenience.
- Не возвращать legacy session-only writeoff как допустимый новый production flow.

## Последний кодовый checkpoint перед pause-документацией

```text
ef0e64838ebb3f0519f6bfe756ade599a07450b9  Require exact run provenance in writeoff UI
```

После него handoff-коммиты сдвигают HEAD документацией. При продолжении сначала определить фактический current HEAD.

Последний exact-run provenance блок:

```text
c7335631c660a7b5ee71da880a1f77e4e5faa83f  Require exact run provenance for new wire writeoffs
7b92010294342d2a9cc9a153f306673f5c66ffb9  Require run provenance in wire writeoff API
ef0e64838ebb3f0519f6bfe756ade599a07450b9  Require exact run provenance in writeoff UI
```

Duplicate `/api/winding-history` bootstrap удалён:

```text
75207162c7164121252057391230077d5340be3a
```

## CI факт

Историческая compile failure:

```text
CM_MaterialLedger.cpp: expected '}' at end of input
```

исправлена:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8
```

Текущий head **не считать GREEN**, пока нет фактического успешного ESP32 Actions/build result. Connector не показывает общий список push-triggered runs.

## Текущая точка продолжения

Repo-level production flow собран примерно на 90% эксплуатационной готовности. Следующее repo-only действие — проверить фактический build/link и точное наличие definitions:

```text
WarehouseStore::confirmedWriteOffForSourceRun(...)
WarehouseStore::confirmedWriteOffForSourceSession(...)
```

Не создавать второй symbol по предположению. Если build/link чистый — переходить к обязательному ESP32 + Arduino hardware E2E и Stage 0 benchmark; новых observability metrics до benchmark не добавлять.

Все свежие детали: `13_PAUSE_HANDOFF_2026-08-08_2222.md`.
