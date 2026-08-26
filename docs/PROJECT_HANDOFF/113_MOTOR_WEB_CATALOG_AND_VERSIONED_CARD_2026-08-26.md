# 113 — Motor Web catalog + versioned card

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Статус

```text
MOTOR WEB PART A — GREEN
```

## Реализовано

### Каталог

`firmware/esp32/web/desktop/motors.html`

- каталог теперь read-only: inline создание удалено;
- добавлен переход на отдельную страницу `/desktop/motor-new.html`;
- сохранены поиск и bounded cursor pagination;
- строки каталога показывают:
  - двигатель;
  - фазы;
  - WORKING program/repeat;
  - STARTING program/repeat;
  - conductor/material summary;
  - действия;
- для versioned motors используется существующий read-only API:
  `GET /api/motors/winding/latest?motor_id=...`;
- если версии нет, legacy `coil_program + repeat_target` явно показывается как `legacy WORKING`;
- STARTING при отсутствии остаётся `—`.

### Создание двигателя

Новый файл:

`firmware/esp32/web/desktop/motor-new.html`

- отдельная форма создания двигателя;
- сохранены manufacturer/model/phase_count/slot_count/coil_program/repeat_target;
- сохранена проверка похожих карточек;
- после успешного POST открывается карточка созданного двигателя;
- legacy WORKING остаётся совместимым до появления отдельного write API версий;
- физический START остаётся только локальным, Web не управляет SSR.

### Карточка двигателя

`firmware/esp32/web/desktop/motor-details.html`

- current version через `GET /api/motors/winding/latest`;
- paged version history через `GET /api/motors/winding/versions`;
- отдельные WORKING / STARTING блоки;
- multi-conductor canonical strings отображаются напрямую;
- legacy fallback синтезирует только WORKING и не выдумывает STARTING;
- сохранена bounded repair history;
- сохранены ссылки на winding history / costing.

## Regression

Обновлены:

- `Tests/Web/check_motor_schema_ui.js`
- `Tests/Web/check_motor_details_ui.js`

Regression теперь защищает split catalog/create, versioned lookup, WORKING/STARTING, legacy fallback и physical START/SSR safety wording.

## Коммиты

```text
239a4dabb6155dfc0146ed1c6abad004ceb31053  feat(web): add dedicated motor creation page
49da790ec7d1e6b356243b878455c4d79ba2a18a  feat(web): make motor catalog read-only
310d22558b57486d8ed41b66032b66fcae1a0c26  test(web): protect dedicated motor creation flow
743451203afbf8613a12ae7a5a10dd8ee7a09d08  feat(web): show versioned motor winding card
39770f0760c3750b7352a1abec65b4740424c304  test(web): protect versioned motor detail card
```

## Проверка

```text
CMP 32932380926 / SUCCESS
CMP 32932518963 / SUCCESS
```

ESP32 production C++ в этом подблоке не изменялся, поэтому новый ESP32 Build для Part A не требуется и не заявляется.

## NEXT

1. Добавить в motor working card контекст AS_RECEIVED / after rewinding по repair snapshots.
2. Проверить существующий linked-job flow и добавить role-aware WORKING/STARTING handoff без обхода exact repair/spool/snapshot safety.
3. Не создавать отдельный небезопасный shortcut, который POST-ит JOB в обход текущего linked-job preflight.
4. После role-aware job flow закрыть Motor Web и перейти к Client Web.
