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

Текущая работа — final feature-completeness / runtime acceptance audit. Исправлять только подтверждённые пробелы. Не переоткрывать закрытые блоки по предположению.

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
- desktop/mobile repair creation legacy handoff by exact `client_id` / `motor_id`;
- settings network/time/Hall/FTP runtime HTML escaping;
- pricing-audit runtime HTML escaping;
- desktop motor catalog versioned conductor HTML escaping.

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

### Runtime DOM-boundary checkpoints

The final acceptance audit found several real presentation-layer gaps where server/runtime strings were inserted into `innerHTML` without browser-side HTML escaping. Backend JSON escaping remains transport protection and is not a substitute for DOM escaping.

Checkpoint 29 closed the main settings network summary. Checkpoint 30 extended the same boundary to time/RTC, Hall and FTP/Web-recovery runtime status. Checkpoint 31 closed the pricing-audit status/timestamp path. Checkpoint 32 closed raw versioned `working_conductors` / `starting_conductors` in desktop `motors.html`; mobile catalog already escaped conductor text.

Exact verified chain:

```text
CMP #4864  run 33734195479 / SUCCESS
ESP32 #1884 run 33734113844 / SUCCESS
ESP32 #1885 run 33734170099 / SUCCESS
CMP #4865  run 33734386839 / SUCCESS
CMP #4868  run 33734667118 / SUCCESS
CMP #4871  run 33734862776 / SUCCESS
CMP #4872  run 33734956028 / SUCCESS
Reference #126 run 33734956275 / SUCCESS
ESP32 #1890 run 33734955992 / SUCCESS
CMP #4873  run 33734985565 / SUCCESS
CMP #4874  run 33735201736 / SUCCESS
CMP #4877  run 33735336317 / SUCCESS
CMP #4878  run 33735432605 / SUCCESS
CMP #4881  run 33736069877 / SUCCESS
Reference #129 run 33736069966 / SUCCESS
ESP32 #1893 run 33736069916 / SUCCESS
CMP #4882  run 33736114112 / SUCCESS
```

Detailed records:

```text
docs/PROJECT_HANDOFF/29_CHECKPOINT_SETTINGS_NETWORK_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/30_CHECKPOINT_SETTINGS_RUNTIME_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/31_CHECKPOINT_PRICING_AUDIT_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/32_CHECKPOINT_MOTOR_CATALOG_CONDUCTOR_ESCAPING_2026-09-03.md
```

Regression coverage protects the affected DOM boundaries through the existing Web contract graph, including `check_settings_hub_parity.js` and `check_motor_edit_ui.js`.

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
- desktop/mobile `winding-history.html` escape event/session/run/job/motor values; `shared/winding-history-spools.js` escapes spool/session/wire values — **NO-CHANGE**;
- `service-job.html` desktop/mobile uses `textContent` for runtime state and retains strict cancel/dismiss restrictions — **NO-CHANGE**;
- shared Wi-Fi settings already escape SSID/IP/runtime strings — **NO-CHANGE**;
- desktop/mobile backup export already protect dynamic file/session/error rendering — **NO-CHANGE**;
- desktop/mobile material catalog already escape names/units/currency/comments/timestamps and encode IDs in links — **NO-CHANGE**;
- Arduino winding archive shared UI already escapes dynamic archive/motor/program text and normalizes numeric IDs — **NO-CHANGE**;
- desktop/mobile Cash UI escapes client/motor/payment/comment/runtime values and remains append-only — **NO-CHANGE**;
- desktop `motor-details.html` uses `textContent` or escaped `formatConductors()` output for winding fields/history — **NO-CHANGE**;
- `shared/writeoff-spool-suggestion.js` preserves exact RUN/session/spool/material-request identity and safely renders runtime strings — **NO-CHANGE**;
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

For Web rendering, continue the explicit rule: every server/runtime string interpolated into `innerHTML` must be escaped; prefer `textContent` when markup is not required.

If inspection finds no defect in an area, record **NO-CHANGE** rather than inventing work.

Do not resume speculative Uno parser/compiler micro-optimization unless measured Flash/RAM pressure again justifies it. Do not promote deferred backlog items without a new explicit request or concrete current evidence.

## Read order for a new chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/32_CHECKPOINT_MOTOR_CATALOG_CONDUCTOR_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/31_CHECKPOINT_PRICING_AUDIT_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/30_CHECKPOINT_SETTINGS_RUNTIME_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/29_CHECKPOINT_SETTINGS_NETWORK_HTML_ESCAPING_2026-09-03.md
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
