# Checkpoint — motor WORKING / STARTING editing

Дата: **2026-08-31**  
Рабочая ветка: **`arduino-ru-lcd-experiment`**  
Production `cmp-protocol-v1` не изменён.

## Цель

Добавить редактирование канонических ролей обмотки `WORKING` и `STARTING` непосредственно в desktop/mobile карточку редактирования двигателя без изменения существующей истории.

## Реализованный контракт

- authoritative read: `GET /api/motors/winding/latest?motor_id=...`;
- mutation: `POST /api/motors/winding/role`;
- optimistic concurrency token: `expected_winding_version_id`, равный загруженному `winding_version_id`;
- backend непосредственно перед append повторно читает latest version для exact `motor_id`;
- stale expected version возвращает HTTP `409` + `winding_version_conflict` и текущий `current_winding_version_id`;
- при `409` desktop/mobile перечитывают latest и не выполняют автоматический повтор записи;
- изменение одной роли копирует вторую роль из latest без изменений;
- `WORKING` остаётся обязательной канонической ролью;
- первая versioned запись может быть создана через `WORKING` с `expected_winding_version_id=0`;
- `STARTING` без существующей `WORKING` versioned записи fail-closed с `409 working_winding_version_required`;
- `STARTING` можно явно отключить, что создаёт новую версию с `starting_present=false`;
- проводники вводятся в уже используемом каноническом формате, например `CU:95x1+CU:100x1`, максимум 4 компонента;
- новая запись получает `previous_version_id` текущей latest версии и `version_kind=MANUAL_ROLE_EDIT`;
- storage остаётся append-only `motor-winding-versions.ndjson`; существующие строки не изменяются и не удаляются.

## UI

Изменены:

```text
firmware/esp32/web/desktop/motor-edit.html
firmware/esp32/web/mobile/motor-edit.html
```

В секции «Обмотка» добавлены отдельные редакторы:

- «Рабочая обмотка»;
- «Пусковая обмотка»;
- текущий `winding_version_id`;
- отдельные кнопки сохранения каждой роли;
- сообщение о конфликте и refresh latest.

Существующее редактирование паспортных/legacy motor fields через `/api/motors/update` сохранено отдельно и не используется как способ перезаписи canonical winding history.

## Backend

Изменены:

```text
firmware/esp32/src/CM_MotorSimilarityWeb.h
firmware/esp32/src/CM_MotorSimilarityWeb.cpp
```

`MotorSimilarityWeb` теперь владеет отдельным `MotorWindingVersionStore` для mutation endpoint. GET latest остаётся в `RepairRegistryLookupWeb`.

## Regression coverage

`Tests/Web/check_motor_edit_ui.js` теперь проверяет:

- наличие role-edit endpoint;
- `expected_winding_version_id`;
- `409` conflict contract;
- `previousVersionId` linkage;
- `MANUAL_ROLE_EDIT` provenance;
- независимую замену WORKING/STARTING;
- fail-closed STARTING-first guard;
- desktop/mobile latest-load, role-save и conflict-refresh;
- отсутствие SSR control в motor editor.

## CI state

На промежуточном feature HEAD:

```text
43d405c40b5cc042a4bc78e92340a8546d031c73
CMP Protocol Tests #4651
run 33355313297 / SUCCESS
```

Это не объявляет последующие documentation commits GREEN. Финальный exact HEAD должен получить собственный applicable CI result перед фиксацией как GREEN.

## Safety invariants

Не изменены:

- automatic physical START отсутствует;
- auto-resume после reboot отсутствует;
- ESP32/Web не управляет SSR напрямую;
- `RUN_COMPLETED` сам по себе не списывает провод;
- RUN_WIRE write-off остаётся manual и exact-linked;
- append-only winding evidence не редактируется in-place;
- production `cmp-protocol-v1` не изменён.
