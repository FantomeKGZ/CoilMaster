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
- `material_not_found`;

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

Не создавать новые ledgers. Проверить только operator visibility в единой карточке «Материалы ремонта»:

1. может ли оператор из одной карточки однозначно сопоставить generic usage с его append-only correction history и увидеть net quantity/cost;
2. достаточно ли видна отдельная RUN_WIRE provenance/history без смешивания warehouse и generic ledgers;
3. есть ли конкретное действие при `material_not_found`/отсутствующей складской позиции (переход в существующий склад/создание позиции, только если текущий workflow это безопасно разрешает);
4. быстрый список часто используемых/подходящих позиций добавлять только если его можно построить bounded/read-only без нового дублирующего состояния.

Hardware E2E остаётся финальным отдельным gate после software-аудита.

## Safety invariants

Без изменений: no automatic START, no auto-resume, Arduino sole SSR owner, RUN_COMPLETED evidence-only, exact RUN_WIRE spool/session/run provenance, manual writeoff, fail-closed restore/recovery, no unbounded NDJSON buffering/automatic truncation/early DB migration.
