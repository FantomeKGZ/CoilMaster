# Checkpoint 150 — Material usage fail-closed operator UX

Дата: **2026-08-28**  
Рабочая ветка: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1`** — не изменён.

## Что закрыто

Обычное ручное списание материалов ремонта теперь даёт оператору конкретное fail-closed действие для ошибок authoritative/idempotency preflight, не добавляя второго mutation endpoint и не выполняя автоматический retry.

Runtime/UI commit:

```text
28343cb20cc5d75c748c38881d8705fd8505d182
feat(materials): clarify fail-closed usage recovery
```

Contract commit:

```text
eff0cb8a7e0365cf6184aa81694d9e277037116d
test(materials): lock fail-closed usage guidance
```

Shared controller `firmware/esp32/web/shared/material-usage-corrections.js` оборачивает уже существующий `usageForm.onsubmit` только для классификации ответа `POST /api/materials/usage`. Сам controller не создаёт второй generic material POST.

## Семантика operation_id

Одинаковый `operation_id` сохраняется для fail-closed/неоднозначных ответов:

- `material_state_unavailable_after_replay`;
- `usage_idempotency_read_failed`;
- `repair_reference_read_failed`;
- `repair_lifecycle_unavailable`;
- `material_reference_read_failed`;
- `materials_unavailable`.

Особенно важно: `material_state_unavailable_after_replay` приходит после найденного durable replay. UI явно сообщает, что операция с этим `operation_id` **могла уже быть сохранена**, запрещает создавать новый id/менять строку и оставляет только ручной повтор с тем же id после authoritative reread.

Для доказанных pre-mutation no-write результатов:

- `invalid_material_preview`;
- `material_not_found`.

UI использует существующий stale-preview путь: сбрасывает старый id, перечитывает каталог и требует нового ручного выбора/подтверждения.

Автоматической повторной отправки нет.

## Append-only corrections — фактическое состояние

Аудит актуального дерева подтвердил, что следующий пункт старого `06_ACTIVE_WORK_AND_NEXT_STEPS.md` про реализацию append-only correction/history уже устарел: эта архитектура в ветке существует и покрыта обязательным contract-test.

Текущие authoritative модули:

```text
CM_MaterialUsageCorrection.cpp
CM_MaterialUsageCorrectionHistory.cpp
CM_MaterialUsageCorrectionIntegrityAudit.cpp
CM_MaterialUsageCorrectionWeb.cpp
```

Действующий контракт `Tests/Web/check_material_usage_correction_contracts.js` подтверждает:

- immutable source usage — подтверждённая строка не редактируется/не удаляется;
- отдельную append-only correction запись с exact `source_usage_id`/`repair_id`/quantity/operation id;
- idempotent replay и conflict semantics;
- cumulative over-correction guard;
- persisted source price snapshot для correction cost;
- отдельный запрет корректировать RUN_WIRE через generic material correction;
- bounded correction history;
- integrity/backup coverage;
- net `RepairCosting = confirmed generic usage - append-only generic corrections`.

Поэтому повторно реализовывать correction ledger нельзя.

## Operator visibility audit

### Generic usage / corrections

Карточка уже показывает:

- исходный `usage_id`, material id, quantity, persisted unit-price snapshot, line cost, timestamp/comment;
- отдельную correction history, где каждая запись содержит exact `source_usage_id`, returned quantity, correction line cost и timestamp/comment;
- общий authoritative net material cost через `RepairCosting`, включая отдельную сумму/count append-only corrections.

Mutation-time `correctUsage()` уже вычисляет cumulative corrected quantity и `remainingCorrectableQuantityMilli` по exact source usage. Делать второй read API с повторным полным scan только ради постоянного per-row net label сейчас нецелесообразно. Точный remaining correction по выбранной операции остаётся authoritative на сервере и возвращается при over-correction/duplicate replay.

### RUN_WIRE

Отдельный `writeoff-spool-suggestion.js` уже показывает оператору exact provenance без смешивания ledgers:

```text
active pending run:
source_session_id + source_run_id + immutable spool_id

history row:
movement_id + spool_id + mass/value + source_session_id + source_run_id
```

Legacy movement без `source_run_id` явно маркируется как legacy и не маскируется под exact current provenance. Current RUN_WIRE submit остаётся ручным и содержит exact session/run/spool/diameter/material identity.

Новый RUN_WIRE history API или перенос его в generic correction ledger не нужен.

### Missing material navigation

Существующие безопасные страницы подтверждены:

```text
/desktop/material-catalog.html -> /desktop/material-new.html
/mobile/material-catalog.html  -> /mobile/material-new.html
```

То есть при `material_not_found` можно позже добавить только явную ссылку «Открыть каталог / Создать материал». Автоматически создавать материал нельзя. Это UX-polish, не integrity blocker.

## Verification — GREEN

```text
CMP Protocol Tests #3867
run 33164535872
SHA eff0cb8a7e0365cf6184aa81694d9e277037116d
completed / success

ESP32 Build #1700
run 33164496835
SHA 28343cb20cc5d75c748c38881d8705fd8505d182
completed / success

Arduino RU LCD Build #124
run 33164496837
SHA 28343cb20cc5d75c748c38881d8705fd8505d182
completed / success
```

## Следующий реальный software audit

Core accounting/integrity material flow считать закрытым на текущем software checkpoint. Не создавать новые ledgers.

Оставшийся порядок:

1. небольшой UX-polish `material_not_found` -> явная desktop/mobile навигация к существующему material catalog/create workflow;
2. отдельно проверить, нужен ли bounded/read-only shortcut часто используемых материалов; не добавлять новый persisted favourites/state без необходимости;
3. после этого перейти к общему software acceptance audit по material plan;
4. hardware Arduino+ESP32 E2E оставить финальным отдельным gate.

## Safety invariants

Без изменений: no automatic START, no auto-resume, Arduino sole SSR owner, RUN_COMPLETED evidence-only, exact RUN_WIRE spool/session/run provenance, manual writeoff, fail-closed restore/recovery, no unbounded NDJSON buffering/automatic truncation/early DB migration.
