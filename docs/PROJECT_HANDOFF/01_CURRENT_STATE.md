# Текущее состояние CoilMaster

Дата обновления: **2026-09-03**  
Рабочая/source-of-truth ветка: **`cmp-protocol-v1`**  
Stable/ready ветка: **`main`**

## Source of truth / branch policy

Текущая схема разработки:

```text
cmp-protocol-v1 = единственная активная development/source ветка
main            = stable/ready checkpoint branch
```

`arduino-ru-lcd-experiment` retired и больше не использовать как source.

Перед изменением существующего файла обязательно получить актуальное содержимое из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала подтвердить отсутствие пути. CI нельзя называть GREEN без exact `completed/success` для соответствующего HEAD.

## Current phase

Проект находится в final feature-completeness / runtime-acceptance audit после большого блока реализации и оптимизации.

Главное правило текущей фазы: исправлять только подтверждённые дефекты или реально отсутствующие обещанные функции. Если область уже работает и защищена regression tests, фиксировать **NO-CHANGE**, а не создавать новую переработку.

## Current production flow

```text
client
→ motor
→ OPEN repair
→ costing
→ linked winding WORKING/STARTING
→ exact spool selection
→ immutable snapshot
→ UART delivery
→ physical Arduino START
→ RUN_STARTED / RUN_COMPLETED evidence
→ manual exact RUN_WIRE writeoff
→ costing
→ finalization preflight
→ CLOSED
→ optional delivery evidence
→ reports
→ backup
```

`CLOSED` и физическая выдача клиенту остаются раздельными состояниями. Delivery evidence append-only/readable из CRM history; баланс/касса не блокирует выдачу автоматически.

## Safety ownership

Неизменяемые текущие границы:

- physical START только локально на Arduino;
- никакого automatic START или automatic repeat START;
- Arduino остаётся единственным владельцем SSR;
- ESP32/Web не управляют SSR напрямую;
- после reboot нет auto-resume физического движения;
- `RUN_COMPLETED` сам по себе не списывает провод;
- ручное RUN_WIRE списание требует exact `spool_id + source_session_id + source_run_id`;
- historical/mutation evidence append-only;
- restore operator-controlled и fail-closed;
- mutation-time TOCTOU rereads сохраняются там, где они являются safety boundary;
- никаких автоматических destructive rotation/truncation production data;
- без измеримой необходимости не вводить persistent DB/index/cache migration.

## Arduino Uno current state

Production Uno успешно собирается с обязательным CI headroom guard:

```text
MIN_HEADROOM_BYTES = 512
```

Последний измеренный code checkpoint:

```text
Flash: 31114 / 32256; free 1142 bytes
RAM:   1227 / 2048;   free 821 bytes
```

Последняя подтверждённая цепочка после Flash cleanup:

```text
CMP #4820  run 33727723542 / SUCCESS
Uno #256   run 33727723569 / SUCCESS
CMP #4821  run 33727817692 / SUCCESS
Uno #257   run 33727817804 / SUCCESS
CMP #4822  run 33728065920 / SUCCESS
```

Не продолжать speculative Uno micro-optimization без нового измеримого давления Flash/RAM.

## Arduino / Hall / local controls

Current Hall architecture сохраняет local physical ownership:

- ESP32/Web может ARM/refresh/abort/process calibration data;
- начало движения остаётся через keypad `A` или physical START на Arduino;
- Web не имеет Hall motor-start endpoint;
- Web не получает SSR control;
- calibration run 15 s с bounded timeout/peer guards;
- ESP32 анализирует данные и предлагает settings;
- сохранение/применение остаётся отдельной подтверждаемой операцией на Arduino;
- RU LCD Hall states и bounded custom CGRAM защищены regression contracts.

Текущий buzzer pin/config:

```text
A3 = buzzer + for direct small active buzzer
GND = buzzer -
activeHigh = true
```

## ESP32 / Web current state

ESP32 остаётся data/network/UI controller, а не safety controller.

Подтверждённые крупные области:

- clients / motors / repairs CRM;
- dedicated create/edit/details pages;
- exact client/motor handoff into repair creation;
- bounded cursor pagination;
- motor import;
- canonical append-only motor winding versions;
- WORKING mandatory and optional STARTING;
- conductor material CU/AL + 1–5 physical wire diameters;
- exact version conflict protection;
- autonomous Arduino archive/history;
- linked winding job preparation;
- warehouse/spools/materials;
- manual exact RUN_WIRE writeoff;
- costing and finalization;
- CLOSED vs DELIVERED separation and delivery history visibility;
- cash/payment append-only journal;
- reports based on authoritative costing, not a second cash-derived costing source;
- Wi-Fi profiles/static IP/fallback AP/`coil.local`;
- FTP `/web` recovery;
- backup/restore;
- diagnostics;
- reference SD bundle/search.

## Recent CRM / Web acceptance checkpoints

### Delivery history

Client/motor history now exposes closed-repair delivery status read-only while delivery mutation remains in the repair workflow.

Exact verification chain:

```text
CMP #4853      run 33732645388 / SUCCESS
Reference #118 run 33732645370 / SUCCESS
ESP32 #1882    run 33732645361 / SUCCESS
CMP #4854      run 33732693018 / SUCCESS
```

### Repair creation handoff parity

Legacy repair links carrying `client_id` and/or `motor_id` now enter the dedicated matching `repair-new.html` page on desktop/mobile without reinterpreting `client_id` as a hidden mobile catalog filter.

Exact verification:

```text
CMP #4856      run 33733146747 / SUCCESS
Reference #119 run 33733146680 / SUCCESS
ESP32 #1883    run 33733146716 / SUCCESS
CMP #4857      run 33733174978 / FAILURE
  intermediate regression test was implementation-specific, runtime handoff remained valid
CMP #4858      run 33733234029 / SUCCESS
CMP #4859      run 33733410301 / SUCCESS
```

Detailed current records:

```text
docs/PROJECT_HANDOFF/27_CHECKPOINT_MOTOR_DELIVERY_HISTORY_2026-09-03.md
docs/PROJECT_HANDOFF/28_CHECKPOINT_REPAIR_CREATION_HANDOFF_PARITY_2026-09-03.md
```

## Runtime HTML/DOM boundary state

Final acceptance audit found and closed real presentation-layer gaps where runtime/server strings were placed into `innerHTML` without browser-side escaping.

Closed areas:

- main settings network summary;
- time / RTC status;
- Hall source/reply status;
- FTP / Web-recovery runtime addresses/result;
- pricing-audit status and raw timestamp fallback.

Current rule:

```text
server/runtime string + innerHTML => HTML escape first
plain text rendering             => prefer textContent
form value                       => assign through .value
```

Backend JSON escaping protects JSON transport and does not replace DOM escaping.

Exact verification chain:

```text
CMP #4864      run 33734195479 / SUCCESS
ESP32 #1884    run 33734113844 / SUCCESS
ESP32 #1885    run 33734170099 / SUCCESS
CMP #4865      run 33734386839 / SUCCESS
CMP #4868      run 33734667118 / SUCCESS
CMP #4871      run 33734862776 / SUCCESS
CMP #4872      run 33734956028 / SUCCESS
Reference #126 run 33734956275 / SUCCESS
ESP32 #1890    run 33734955992 / SUCCESS
CMP #4873      run 33734985565 / SUCCESS
CMP #4874      run 33735201736 / SUCCESS
CMP #4877      run 33735336317 / SUCCESS
CMP #4878      run 33735432605 / SUCCESS
```

Detailed records:

```text
docs/PROJECT_HANDOFF/29_CHECKPOINT_SETTINGS_NETWORK_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/30_CHECKPOINT_SETTINGS_RUNTIME_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/31_CHECKPOINT_PRICING_AUDIT_HTML_ESCAPING_2026-09-03.md
```

## Web regression reachability

Checkpoint 21 closed the remaining orphan regression issue:

- `check_cash_ui.js` restored to normal Web audit graph;
- CI now fails if any `Tests/Web/check_*.js` is unreachable;
- guard exposed the only remaining orphan `check_ru_hall_calibration_experiment.js`;
- that test was confirmed active and retained;
- stale desktop/mobile `statistics.html` placeholders remain absent.

Exact verification:

```text
CMP #4823  run 33728359779 / SUCCESS
CMP #4824  run 33728459826 / FAILURE
  intentional audit discovery: RU Hall regression orphan
CMP #4825  run 33728574447 / SUCCESS
CMP #4826  run 33728672746 / SUCCESS
```

Detailed record:

```text
docs/PROJECT_HANDOFF/21_CHECKPOINT_WEB_REGRESSION_REACHABILITY_2026-09-03.md
```

## Motor / winding questionnaire state

New motor creation and edit/detail flows are complete at repo/CI level:

- WORKING winding mandatory;
- optional STARTING;
- material CU/AL separately for each role;
- 1–5 physical conductors;
- `;` or `:` separators;
- decimal comma or dot;
- duplicate diameters aggregate through canonical `xN` representation;
- legacy `coil_program` and `repeat_target` derive from WORKING;
- optimistic append token `expected_winding_version_id`;
- stale token => `409 winding_version_conflict`;
- no silent overwrite/retry.

Detailed records:

```text
docs/PROJECT_HANDOFF/17_CHECKPOINT_WORKING_STARTING_EDIT_2026-08-31.md
docs/PROJECT_HANDOFF/18_CHECKPOINT_MOTOR_NEW_WINDING_CAPTURE_2026-09-03.md
```

## Client creation state

Client questionnaire complete at repo/CI level:

- name required;
- phone required and normalized/validated;
- comment optional;
- creation does not automatically create repair/motor relation;
- desktop/mobile handoff to repair flow is complete;
- mobile may open the client card first; that card exposes `+ Новый ремонт` with exact `client_id`.

## Latest NO-CHANGE acceptance checks

Current `cmp-protocol-v1` inspection found no code change required in these areas:

- desktop/mobile `motor-new` post-create flow;
- mobile client-new → client card → exact client-scoped repair creation;
- desktop/mobile motor card → exact motor-scoped repair creation;
- desktop/mobile reports semantics and pagination;
- report financial aggregation remains intentionally based on authoritative repair costing;
- desktop/mobile `service-job.html` runtime rendering and cancel/dismiss restrictions;
- shared Wi-Fi settings runtime rendering;
- desktop/mobile backup export rendering and safety gating;
- desktop/mobile material catalog dynamic rendering;
- Arduino winding archive dynamic rendering and exact run provenance;
- no current repository-search hit for user-facing `TODO`, `FIXME`, `not implemented`, `готовится` or `заглушка`.

Cosmetic desktop/mobile differences alone are not treated as defects.

## Growing NDJSON / performance state

Previously confirmed repeated-scan optimizations are closed. Remaining deliberate full scans are retained where needed for:

- mutation-time safety;
- correction/provenance history;
- recovery boundaries;
- arbitrary cross-domain reference validation.

Do not restart generic repeated-scan cleanup solely because a file is opened more than once. Persistent indexing/DB migration remains deferred until real size/latency/RAM measurements justify it.

## Current documentation state

Current entry documents:

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

Historical checkpoint files may still mention the old experiment branch because they document the state at that time; do not rewrite historical evidence merely for wording consistency.

## Current NEXT

Continue current `cmp-protocol-v1` final acceptance audit and only change code for a proven defect/incomplete behavior.

Priority order:

1. concrete current CI/runtime failure;
2. user-facing function that repository inspection proves incomplete/broken;
3. previously promised feature that is genuinely absent;
4. explicit new user request.

For Web UI, continue auditing dynamic `innerHTML` paths and require escaping for server/runtime strings. Prefer `textContent` when markup is unnecessary.

If no defect is found in a reviewed area, record **NO-CHANGE** and move on. Deferred backlog is not an automatic task queue.

External two-board/hardware smoke remains a separate physical verification gate and can never be inferred from GitHub CI.

## Key handoff records

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/31_CHECKPOINT_PRICING_AUDIT_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/30_CHECKPOINT_SETTINGS_RUNTIME_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/29_CHECKPOINT_SETTINGS_NETWORK_HTML_ESCAPING_2026-09-03.md
docs/PROJECT_HANDOFF/28_CHECKPOINT_REPAIR_CREATION_HANDOFF_PARITY_2026-09-03.md
docs/PROJECT_HANDOFF/27_CHECKPOINT_MOTOR_DELIVERY_HISTORY_2026-09-03.md
docs/PROJECT_HANDOFF/21_CHECKPOINT_WEB_REGRESSION_REACHABILITY_2026-09-03.md
docs/PROJECT_HANDOFF/20_CHECKPOINT_BRANCH_POLICY_AND_UNO_HEADROOM_2026-09-03.md
docs/PROJECT_HANDOFF/18_CHECKPOINT_MOTOR_NEW_WINDING_CAPTURE_2026-09-03.md
docs/PROJECT_HANDOFF/17_CHECKPOINT_WORKING_STARTING_EDIT_2026-08-31.md
docs/PROJECT_HANDOFF/13_HALL_RU_LCD_ACCEPTANCE.md
docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```
