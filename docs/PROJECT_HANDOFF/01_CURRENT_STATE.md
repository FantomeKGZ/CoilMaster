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

Проект находится в feature-completeness / runtime-audit фазе после большого блока реализации и оптимизации.

Главное правило текущей фазы: исправлять только подтверждённые дефекты или реально отсутствующие обещанные функции. Если область уже работает и защищена regression tests, фиксировать NO-CHANGE, а не создавать новую переработку.

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
→ reports
→ backup
```

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
- cash/payment append-only journal;
- reports;
- Wi-Fi profiles/static IP/fallback AP/`coil.local`;
- FTP `/web` recovery;
- backup/restore;
- diagnostics;
- reference SD bundle/search.

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
- desktop/mobile parity and handoff to repair flow covered by regression tests.

## Growing NDJSON / performance state

Previously confirmed repeated-scan optimizations are closed. Remaining deliberate full scans are retained where needed for:

- mutation-time safety;
- correction/provenance history;
- recovery boundaries;
- arbitrary cross-domain reference validation.

Do not restart generic repeated-scan cleanup solely because a file is opened more than once. Persistent indexing/DB migration remains deferred until real size/latency/RAM measurements justify it.

## Current documentation state

Entry documents are being synchronized to the new branch policy:

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

Historical checkpoint files may still mention the old experiment branch because they document the state at that time; do not rewrite historical evidence merely for wording consistency.

## Current NEXT

Continue current `cmp-protocol-v1` audit and only change code for a proven defect/incomplete behavior.

Priority order:

1. concrete current CI/runtime failure;
2. user-facing function that repository inspection proves incomplete/broken;
3. previously promised feature that is genuinely absent;
4. explicit new user request.

Current stale/empty Web sweep found no new proven user-facing placeholder after removal of Statistics and regression-reachability cleanup. Normal HTML input `placeholder=` attributes are not unfinished pages.

If no defect is found in a reviewed area, record NO-CHANGE and move on.

## Key handoff records

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/21_CHECKPOINT_WEB_REGRESSION_REACHABILITY_2026-09-03.md
docs/PROJECT_HANDOFF/20_CHECKPOINT_BRANCH_POLICY_AND_UNO_HEADROOM_2026-09-03.md
docs/PROJECT_HANDOFF/18_CHECKPOINT_MOTOR_NEW_WINDING_CAPTURE_2026-09-03.md
docs/PROJECT_HANDOFF/17_CHECKPOINT_WORKING_STARTING_EDIT_2026-08-31.md
docs/PROJECT_HANDOFF/13_HALL_RU_LCD_ACCEPTANCE.md
docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```
