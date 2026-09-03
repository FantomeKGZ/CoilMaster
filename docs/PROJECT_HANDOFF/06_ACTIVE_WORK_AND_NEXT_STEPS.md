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
- orphaned Web regression audit.

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

### Branch-policy documentation sync

`00_READ_FIRST.md` was updated on current development branch to remove the retired `arduino-ru-lcd-experiment` policy and to make `cmp-protocol-v1` the only development/source branch.

Commit:

```text
49442512b8c7f927a295daa5401b941f5e7ee4f9
docs: align current branch policy
```

Its exact CMP run must be checked before using that commit itself as GREEN evidence.

## Current audit findings

User-facing static route validation is already enforced by `check_web_assets.js`.

Additional stale/placeholder sweep currently shows no new proven user-facing placeholder requiring production code change:

- historical `statistics.html` is absent;
- search hits for HTML `placeholder=` are normal form hints, not unfinished pages;
- Hall `Подготовка локального запуска` is an active runtime state, not a placeholder;
- no current `готовится` user-facing stub was found in the repository search.

Do not change a page merely because its filename/content looks old. Require concrete broken/incomplete behavior or an explicit requested feature.

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

Continue the current `cmp-protocol-v1` full feature/runtime audit. Choose the next change only from:

1. a concrete current failure;
2. a proven user-facing/runtime incomplete behavior;
3. a previously promised feature that repository inspection proves is still missing;
4. an explicit new user request.

If inspection finds no defect in an area, record NO-CHANGE rather than inventing work.

Do not resume speculative Uno parser/compiler micro-optimization unless measured Flash/RAM pressure again justifies it.

## Read order for a new chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/21_CHECKPOINT_WEB_REGRESSION_REACHABILITY_2026-09-03.md
docs/PROJECT_HANDOFF/20_CHECKPOINT_BRANCH_POLICY_AND_UNO_HEADROOM_2026-09-03.md
docs/PROJECT_HANDOFF/18_CHECKPOINT_MOTOR_NEW_WINDING_CAPTURE_2026-09-03.md
docs/PROJECT_HANDOFF/17_CHECKPOINT_WORKING_STARTING_EDIT_2026-08-31.md
docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md
docs/PROJECT_HANDOFF/13_HALL_RU_LCD_ACCEPTANCE.md
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```
