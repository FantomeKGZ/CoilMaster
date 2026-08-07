# CoilMaster — продолжение проекта

Дата обновления: 2026-08-07

Репозиторий: `FantomeKGZ/CoilMaster`  
Рабочая ветка: `cmp-protocol-v1`

Этот каталог хранит контекст для переноса разработки в новый чат.

## Запрос для нового чата

> Открой репозиторий `FantomeKGZ/CoilMaster`, работай только с веткой `cmp-protocol-v1`. Сначала прочитай `docs/PROJECT_HANDOFF/01_CURRENT_STATE.md`, затем `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и `09_KEY_FILES_INDEX.md`. После этого обязательно перечитай актуальные версии исходников, которые собираешься менять. Используй остальные handoff-файлы как дополнительную архитектурную и историческую справку. Продолжи с точки из `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и после значимого этапа снова обнови handoff.

## Рекомендуемый порядок чтения

1. `01_CURRENT_STATE.md` — актуальное состояние на момент переноса.
2. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — точная следующая задача.
3. `09_KEY_FILES_INDEX.md` — актуальные ключевые файлы и пути.
4. `08_WORK_RULES_AND_VERIFICATION.md` — правила работы и проверки.
5. `02_ARCHITECTURE_AND_HARDWARE.md` — аппаратная архитектура.
6. `03_PROTOCOL_AND_WINDING_FLOW.md` — протокол и winding flow.
7. `04_DATA_STORAGE_API_UI.md` — данные/API/UI; сверять с текущим кодом, так как часть деталей могла устареть.
8. `05_COMPLETED_WORK_LOG.md` — укрупнённый список уже реализованного.
9. `07_BACKLOG_AND_DEFERRED.md` — отложенные задачи; не считать их реализованными автоматически.
10. `10_SESSION_LOG.md` — журнал последних рабочих сессий.
11. `11_FULL_BRANCH_AUDIT.md` — историческая полная карта ветки; не использовать вместо актуального кода и обновлённых `01/06/09`.

## Источник истины

Приоритет:

1. текущий код ветки `cmp-protocol-v1`;
2. результаты актуальной сборки/тестов;
3. `01_CURRENT_STATE.md` + `06_ACTIVE_WORK_AND_NEXT_STEPS.md`;
4. остальные handoff-файлы;
5. тематические документы `docs/*`;
6. история чатов.

Перед изменением существующего файла всегда заново получать его текущее содержимое и blob SHA из `cmp-protocol-v1`.

Для нового файла сначала проверять отсутствие точного пути.

## Критические напоминания

- Не переключаться на `main`: активная реализация находится в `cmp-protocol-v1`.
- Не начинать заново persistent allocator, immutable snapshot, runtime-state, recovery, linked-job, winding history, workshop registry, warehouse, costing или calculator — они уже существуют.
- ESP32/WEB не должны напрямую управлять SSR.
- Физический START остаётся обязательным.
- Automatic resume/automatic queue после recovery запрещены.
- Не считать документацию доказательством успешной CI-сборки.
- Не считать отсутствие видимого workflow-run доказательством GREEN.
- Не списывать провод автоматически только по `RUN_COMPLETED`, пока не спроектирована устойчивая связь с конкретной складской катушкой и идемпотентность.

## Текущая точка продолжения

На момент этого handoff следующий рабочий блок:

```text
client → motor → repair → linked winding → physical run → winding history
```

Начать с проверки и доработки:

```text
firmware/esp32/web/mobile/repairs.html
firmware/esp32/web/desktop/repairs.html
```

Все детали — в `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
