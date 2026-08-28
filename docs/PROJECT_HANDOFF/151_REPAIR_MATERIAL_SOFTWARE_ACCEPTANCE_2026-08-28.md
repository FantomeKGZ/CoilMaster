# Checkpoint 151 — Repair material software acceptance

Дата: **2026-08-28**  
Рабочая ветка: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1`** — не изменён.

## Итог

Software-блок `docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md` закрыт по core accounting/integrity требованиям. Нового material ledger, favourites store, duplicate costing store или общего RUN_WIRE/generic correction ledger не создано.

Оставшийся `material_not_found -> catalog/create` переход классифицирован как необязательный UX-polish: существующий fail-closed workflow уже перечитывает authoritative каталог и требует нового ручного выбора. Автоматического создания материала нет и добавлять его нельзя.

## Acceptance matrix

### 1. Единая карточка ремонта

Desktop/mobile имеют `Материалы ремонта` и явный вход из карточки ремонта. В одной поверхности доступны:

- обычные материалы MaterialLedger;
- persisted usage history;
- authoritative material costing;
- append-only corrections;
- отдельный видимый переход в dedicated RUN_WIRE workflow.

### 2. Выбор материала

Обычные материалы выбираются из authoritative `/api/materials` без ручного ввода material id. Поиск серверный, bounded/cursor-based, максимум 48 символов, ACTIVE-only, без whole-file result buffering.

Остаток и текущая цена видны до подтверждения. Stale preview и insufficient stock перечитываются authoritative и требуют ручного повторного подтверждения.

Отдельный shortcut «часто используемые» намеренно не добавлен: в текущем дереве нет bounded/read-only aggregation API, а полный scan растущего `usage.ndjson` или новый persisted favourites state не оправданы.

### 3. Generic material writeoff

Списание только явное/manual `POST /api/materials/usage`.

Защиты:

- exact repair/material/quantity;
- persisted price snapshot;
- bounded `operation_id` idempotency;
- duplicate replay без второй мутации;
- operation-id conflict fail-closed;
- authoritative stock/price preflight;
- final mutation-time reread/TOCTOU в `MaterialLedger::confirmUsage()`;
- pending WAL/recovery semantics сохраняются.

### 4. Ambiguous/fail-closed outcome

UI различает:

- доказанный pre-mutation no-write -> refresh + новый manual confirm;
- ambiguous/idempotency/authoritative read failure -> сохраняется тот же `operation_id`;
- durable replay с недоступным current material state -> оператору явно сообщается, что операция могла уже быть сохранена; новый id запрещён;
- automatic retry отсутствует.

### 5. RUN_WIRE

RUN_WIRE остаётся отдельной exact-safe state machine.

Обязательны:

- `RUN_COMPLETED` evidence;
- exact `source_session_id`;
- exact `source_run_id`;
- immutable exact `spool_id`;
- spool -> MaterialLedger bridge;
- material request/status;
- explicit `confirmed=true` operator action.

История показывает movement/spool/mass/value и exact session/run provenance. Legacy entries без run-id помечаются отдельно. Generic materials page не конструирует RUN_WIRE provenance и не вызывает RUN_WIRE mutation endpoint.

### 6. Costing

`RepairCosting` остаётся единственным агрегатором:

- warehouse confirmed movements -> wire cost;
- generic persisted usage snapshots -> material cost;
- append-only generic corrections -> subtract from material cost;
- pricing/labour остаются отдельными существующими источниками.

Старые usage строки не пересчитываются по текущей цене склада.

### 7. History / corrections

Confirmed usage не редактируется и не удаляется.

Generic correction:

- отдельная append-only запись;
- exact `source_usage_id` + repair/material/quantity/operation id provenance;
- persisted source price snapshot;
- cumulative over-correction guard;
- idempotent replay/conflict protection;
- bounded correction history;
- RUN_WIRE correction через generic path запрещена.

### 8. Integrity / backup

Material usage/corrections остаются включены в fail-closed integrity/backup path. Отдельный UI-only compensation layer не создаётся.

Растущие NDJSON не буферизуются целиком, production truncation/rotation автоматически не выполняются, преждевременной DB/index migration нет.

## Последний подтверждённый GREEN runtime/contract checkpoint

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

После этих code/test SHA были только handoff/docs commits; production branch не менялся.

## Deferred / non-blocking UX

Не блокирует software acceptance:

- явные ссылки из `material_not_found` в существующие `material-catalog.html` / `material-new.html`;
- отдельный convenience shortcut часто используемых материалов.

Оба пункта нельзя реализовывать ценой нового persisted state, unbounded scan, автоматического создания/списания или ослабления fail-closed semantics.

## Следующий этап

Material software block считать закрытым. Следующий обязательный этап для этого блока — hardware acceptance на реальных ESP32 + Arduino только когда пользователь перейдёт к hardware E2E.

До hardware gate можно продолжать другие software-задачи ветки `arduino-ru-lcd-experiment`, но material accounting/integrity не расширять без нового конкретного требования.

## Safety invariants

Без изменений:

- no automatic physical START;
- no auto-resume after reboot;
- Arduino — sole SSR owner;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` evidence-only;
- RUN_WIRE manual/exact spool+session+run;
- restore/recovery fail-closed;
- no automatic data truncation/rotation;
- no premature DB/index migration.
