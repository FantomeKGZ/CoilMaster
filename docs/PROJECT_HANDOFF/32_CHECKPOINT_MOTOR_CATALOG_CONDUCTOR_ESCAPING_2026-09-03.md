# Checkpoint 32 — Motor catalog conductor HTML escaping

Дата: **2026-09-03**  
Рабочая/source-of-truth ветка: **`cmp-protocol-v1`**

## Причина

Во время final feature/runtime acceptance audit обнаружен подтверждённый Web DOM-boundary gap только в desktop каталоге двигателей:

```text
firmware/esp32/web/desktop/motors.html
```

Versioned поля:

```text
working_conductors
starting_conductors
```

возвращались `conductorText()` как raw runtime strings и затем включались в `innerHTML` карточки двигателя без HTML escaping.

Mobile catalog уже был безопасен: `roleHtml(...conductors)` использовал `esc(conductors)`.

## Исправление

Desktop `conductorText()` теперь экранирует обе versioned conductor строки до включения в HTML:

```text
Рабочая:  esc(version.working_conductors)
Пусковая: esc(version.starting_conductors)
```

Legacy conductor fallback уже использовал `esc()` и не менялся.

Коммит исправления:

```text
549675b98f19f3d303f438043412df46f42f5ab5
fix(web): escape desktop motor conductors
```

Regression contract усилен в:

```text
Tests/Web/check_motor_edit_ui.js
```

Он теперь требует:

- desktop WORKING conductor escaping;
- desktop STARTING conductor escaping;
- сохранение существующего mobile conductor escaping.

Regression commit:

```text
4eeb1ea5d723c17a36a96efe769c163b25b6012b
test(web): protect desktop motor conductor escaping
```

## Exact CI

Для runtime fix commit `549675b98f19f3d303f438043412df46f42f5ab5` подтверждены:

```text
CMP Protocol Tests #4881
run 33736069877
completed / success

Reference Legacy Import Check #129
run 33736069966
completed / success

ESP32 Build #1893
run 33736069916
completed / success
```

Для regression HEAD `4eeb1ea5d723c17a36a96efe769c163b25b6012b` подтверждён:

```text
CMP Protocol Tests #4882
run 33736114112
completed / success
```

## Дополнительный NO-CHANGE audit

В рамках того же прохода проверены и не потребовали изменений:

- desktop/mobile `winding-history.html`;
- `shared/winding-history-spools.js`;
- desktop/mobile `service-job.html`;
- desktop/mobile Cash UI;
- desktop `motor-details.html`;
- `shared/writeoff-spool-suggestion.js`.

В этих местах runtime/API строки либо проходят `esc()`, либо выводятся через `textContent`; exact spool provenance и RUN_WIRE safety boundaries не изменялись.

## Safety

Не изменялись:

- physical START ownership;
- SSR ownership Arduino;
- no auto-resume;
- manual RUN_WIRE;
- exact `spool_id + source_session_id + source_run_id` provenance;
- append-only motor/winding history;
- costing/payment source-of-truth boundaries.

## NEXT

Продолжать final acceptance audit только по подтверждённым runtime/user-facing gaps. Если следующая область уже корректна — фиксировать **NO-CHANGE**, а не создавать переработку.
