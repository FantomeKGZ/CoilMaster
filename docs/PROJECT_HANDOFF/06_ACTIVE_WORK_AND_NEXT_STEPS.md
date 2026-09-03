# Активная работа и следующие шаги

Дата обновления: **2026-09-03**  
Рабочая/source-of-truth ветка: **`cmp-protocol-v1`**  
Stable/ready ветка: **`main`**

## Branch policy

Текущая обязательная схема:

```text
cmp-protocol-v1 = единственная development/source ветка
main            = stable/ready; обновлять только отдельным подтверждённым promotion
```

`arduino-ru-lcd-experiment` retired и больше не использовать как source.

Перед каждым изменением существующего файла:

1. fetch exact current content из `cmp-protocol-v1`;
2. получить current blob SHA;
3. изменять только относительно этого SHA.

Для нового файла сначала проверить отсутствие пути. Новый HEAD нельзя называть GREEN без exact `completed/success` применимого CI.

## Current active stream

Текущая работа — feature-completeness / runtime audit. Исправлять только подтверждённые пробелы. Не переоткрывать закрытые блоки по предположению.

Закрыты и не являются текущей очередью:

- repair/material/RUN_WIRE safety/accounting foundation;
- repeated-scan optimization checkpoints и их NO-CHANGE closeout;
- autonomous winding → canonical motor history;
- RU Hall LCD/local-control flow;
- WORKING/STARTING canonical role cleanup;
- calculator source strand counts;
- motor import;
- motor WORKING/STARTING edit;
- new-motor questionnaire with canonical WORKING + optional STARTING;
- client creation questionnaire;
- bounded clients/motors/repairs pagination;
- stale Statistics placeholders;
- Cash UI regression reachability;
- orphaned Web regression audit;
- CLOSED/DELIVERED history visibility in client and motor history;
- desktop/mobile repair creation legacy handoff by exact `client_id` / `motor_id`.

## Latest exact verified checkpoints

### Uno resource/headroom checkpoint

Latest measured production Uno code checkpoint:

```text
Flash: 31114 / 32256; free 1142 bytes
RAM:   1227 / 2048;   free 821 bytes
```

The CI guard remains `MIN_HEADROOM_BYTES = 512` for both Flash and RAM. Do not lower the guard to hide growth.

Relevant verified chain:

```text
CMP #4820  run 33727723542 / SUCCESS
Uno #256   run 33727723569 / SUCCESS
CMP #4821  run 33727817692 / SUCCESS
Uno #257   run 33727817804 / SUCCESS
CMP #4822  run 33728065920 / SUCCESS
```

### Web regression reachability checkpoint

Closed findings:

- active `check_cash_ui.js` was orphaned and is now reachable through `check_web_assets.js`;
- CI now fails if any `Tests/Web/check_*.js` is unreachable;
- this guard exposed `check_ru_hall_calibration_experiment.js`;
- inspection confirmed it still protects active RU Hall/local START/SSR/Web safety behavior, so it was retained and made reachable;
- old desktop/mobile `statistics.html` placeholders remain absent.

Exact verification:

```text
CMP #4823  run 33728359779 / SUCCESS
CMP #4824  run 33728459826 / FAILURE
  expected audit discovery: check_ru_hall_calibration_experiment.js orphan
CMP #4825  run 33728574447 / SUCCESS
CMP #4826  run 33728672746 / SUCCESS
```

Detailed record:

```text
docs/PROJECT_HANDOFF/21_CHECKPOINT_WEB_REGRESSION_REACHABILITY_2026-09-03.md
```

### Delivery-history and repair-handoff checkpoints

Recent user-facing parity work is closed and exact-verified:

```text
CMP #4853  run 33732645388 / SUCCESS
Reference #118 run 33732645370 / SUCCESS
ESP32 #1882 run 33732645361 / SUCCESS
CMP #4854  run 33732693018 / SUCCESS
```

This protects CLOSED repair delivery visibility from client/motor history without adding delivery mutation outside the repair workflow.

Repair creation handoff was then aligned so legacy `repairs.html?client_id=...` / `?motor_id=...` links preserve the exact identifiers and enter the matching dedicated `repair-new.html` form on desktop/mobile.

```text
CMP #4856  run 33733146747 / SUCCESS
Reference #119 run 33733146680 / SUCCESS
ESP32 #1883 run 33733146716 / SUCCESS
CMP #4857  run 33733174978 / FAILURE
  expected regression-test defect: test required one specific desktop redirect implementation
CMP #4858  run 33733234029 / SUCCESS
CMP #4859  run 33733410301 / SUCCESS
```

Detailed records:

```text
docs/PROJECT_HANDOFF/27_CHECKPOINT_MOTOR_DELIVERY_HISTORY_2026-09-03.md
docs/PROJECT_HANDOFF/28_CHECKPOINT_REPAIR_CREATION_HANDOFF_PARITY_2026-09-03.md
```

`#4857` is not a runtime regression: the over-specific intermediate test was corrected to verify the actual handoff semantics, and final `#4858` is exact GREEN for that regression HEAD.

### Branch-policy documentation sync

`00_READ_FIRST.md` was updated on current development branch to remove the retired `arduino-ru-lcd-experiment` policy and to make `cmp-protocol-v1` the only development/source branch.

Commit:

```text
49442512b8c7f927a295daa5401b941f5e7ee4f9
docs: align current branch policy
```

## Current audit findings

User-facing static route validation is already enforced by `check_web_assets.js`.

Additional current inspection shows:

- historical `statistics.html` is absent;
- search hits for HTML `placeholder=` are normal form hints, not unfinished pages;
- no current `TODO` / `FIXME` / `not implemented` / `готовится` / `заглушка` user-facing hit was found by repository search;
- desktop/mobile `motor-new` both save canonical WORKING + optional STARTING and finish at the exact new motor card — **NO-CHANGE**;
- mobile `client-new` opens the new client card, whose `+ Новый ремонт` action preserves exact `client_id` — **NO-CHANGE**;
- desktop/mobile motor cards preserve exact `motor_id` when entering repair creation — **NO-CHANGE**;
- desktop/mobile closed-repair reports use the same bounded CLOSED query, registry lookups and authoritative costing source — **NO-CHANGE**;
- cash/payment remains a separate append-only journal and must not be silently introduced as a second costing/report source of truth.

Do not change a page merely because its filename/content looks old or because desktop/mobile UX differs cosmetically. Require concrete broken/incomplete behavior or an explicit requested feature.

## Safety invariants — immutable unless explicitly redesigned

Never weaken:

- physical START only;
- no automatic repeat START;
- Arduino is sole SSR owner;
- ESP32/Web never directly controls SSR;
- no auto-resume after reboot;
- `RUN_COMPLETED` does not auto-writeoff;
- manual RUN_WIRE requires exact `spool_id + source_session_id + source_run_id`;
- exact spool provenance whenever a spool is used;
- append-only historical evidence;
- mutation-time authoritative TOCTOU rereads where required;
- operator/fail-closed restore;
- no automatic production data deletion/rotation;
- no speculative DB/index migration.

## Current NEXT

Continue the current `cmp-protocol-v1` final feature/runtime acceptance audit. Choose the next code change only from:

1. a concrete current failure;
2. a proven user-facing/runtime incomplete behavior;
3. a previously promised feature that repository inspection proves is still missing;
4. an explicit new user request.

If inspection finds no defect in an area, record **NO-CHANGE** rather than inventing work.

Do not resume speculative Uno parser/compiler micro-optimization unless measured Flash/RAM pressure again justifies it. Do not promote deferred backlog items without a new explicit request or concrete current evidence.

## Read order for a new chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/28_CHECKPOINT_REPAIR_CREATION_HANDOFF_PARITY_2026-09-03.md
docs/PROJECT_HANDOFF/27_CHECKPOINT_MOTOR_DELIVERY_HISTORY_2026-09-03.md
docs/PROJECT_HANDOFF/21_CHECKPOINT_WEB_REGRESSION_REACHABILITY_2026-09-03.md
docs/PROJECT_HANDOFF/20_CHECKPOINT_BRANCH_POLICY_AND_UNO_HEADROOM_2026-09-03.md
docs/PROJECT_HANDOFF/18_CHECKPOINT_MOTOR_NEW_WINDING_CAPTURE_2026-09-03.md
docs/PROJECT_HANDOFF/17_CHECKPOINT_WORKING_STARTING_EDIT_2026-08-31.md
docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md
docs/PROJECT_HANDOFF/13_HALL_RU_LCD_ACCEPTANCE.md
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```
