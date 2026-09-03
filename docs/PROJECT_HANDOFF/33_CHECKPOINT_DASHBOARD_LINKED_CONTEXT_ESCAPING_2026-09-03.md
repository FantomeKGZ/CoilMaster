# Checkpoint 33 — Dashboard linked context HTML escaping

Дата: **2026-09-03**  
Ветка: **`cmp-protocol-v1`**

## Подтверждённый дефект

Final runtime/DOM audit обнаружил, что desktop/mobile dashboard при linked winding job вставляли `repair_id` (и на mobile также `motor_id`) из `/api/status` непосредственно в `innerHTML`. `repair_id` одновременно использовался в URL history без `encodeURIComponent()`.

Backend JSON escaping не является защитой DOM boundary, поэтому это закрыто на стороне Web UI.

## Исправление

Desktop `firmware/esp32/web/desktop/index.html`:

- добавлен browser-side `esc()`;
- linked `repair_id` экранируется перед `innerHTML`;
- `repair_id` в history URL проходит `encodeURIComponent()`;
- `motor_id` остаётся plain-text через `textContent`.

Mobile `firmware/esp32/web/mobile/index.html`:

- добавлен browser-side `esc()`;
- linked `repair_id` и `motor_id` экранируются перед `innerHTML`;
- `repair_id` в history URL проходит `encodeURIComponent()`.

Regression `Tests/Web/check_dashboard_job_history.js` теперь требует эти DOM/URL boundaries для обеих поверхностей.

## Коммиты

```text
774a68f29f6c19f042116412a2d2828fc96ed3df  fix(web): escape dashboard linked context
7f939d2d66f532649ce3dee1c9a8565509570e96  fix(web): escape mobile dashboard context
7d3f190a14151dab71b8ba9026bb5e580255365c  test(web): protect dashboard linked context escaping
```

## Exact CI

```text
CMP Protocol Tests #4887
run 33737012053
head 7f939d2d66f532649ce3dee1c9a8565509570e96
completed/success

Reference Legacy Import Check #131
run 33737012006
head 7f939d2d66f532649ce3dee1c9a8565509570e96
completed/success

ESP32 Build #1895
run 33737012189
head 7f939d2d66f532649ce3dee1c9a8565509570e96
completed/success

CMP Protocol Tests #4888
run 33737038921
head 7d3f190a14151dab71b8ba9026bb5e580255365c
completed/success
```

## NO-CHANGE checks

Также проверены без необходимости изменения:

- `firmware/esp32/web/shared/completed-job-display-reset.js` — IDs нормализуются через `positiveInt()`, DOM строится через `textContent`/`replaceChildren`, raw runtime HTML отсутствует;
- desktop/mobile `winding-job.html` — linked IDs/program/spool/status выводятся через `textContent`/`.value`, spool `<option>` создаются DOM API, exact repair/motor/spool linkage и immutable spool selection сохраняются.

## Safety invariants

Не изменены:

- physical START остаётся только локальным;
- никакого automatic START/repeat START;
- Arduino остаётся единственным владельцем SSR;
- ESP32/Web не управляют SSR напрямую;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- linked job сохраняет exact repair/motor/spool provenance;
- manual RUN_WIRE writeoff остаётся отдельной операцией.

## NEXT

Продолжить final Web/runtime acceptance audit по оставшимся динамическим `innerHTML` путям. Исправлять только подтверждённые runtime/server string boundaries или реально неполное поведение; безопасные области фиксировать как **NO-CHANGE**.
