# Checkpoint 34 — Hall current-result text DOM safety

Дата: **2026-09-03**  
Ветка: **`cmp-protocol-v1`**

## Подтверждённый дефект

Final Web/runtime acceptance audit обнаружил отдельный DOM boundary в общем Hall calibration controller:

```text
firmware/esp32/web/shared/settings-hall-calibration.js
```

История калибровок уже строилась безопасно через DOM API и `textContent`, но текущий результат в `renderResult()` собирался через `resultBox.innerHTML` с runtime полями ESP32, включая `recommended_direction`.

Backend JSON transport не заменяет browser-side DOM safety.

## Исправление

Runtime `innerHTML` для текущего результата удалён полностью.

Теперь:

- контейнер начинается через `resultBox.textContent`;
- `VALID` / `INVALID` выделяется созданным DOM `<b>`;
- baseline/min/max/threshold/hysteresis/direction/samples/duration добавляются через `document.createTextNode()`;
- `resultBox.innerHTML=` для runtime результата отсутствует.

Calibration protocol/state machine не менялись.

## Коммиты

```text
d8ce0d41b133d36d6e0f79eac5a79c4266b05cfc  fix(web): render Hall result as text DOM
162e94a9e7539236ff747720cc47850f4156e9f0  test(web): protect Hall result text rendering
```

## Exact CI

```text
CMP Protocol Tests #4890
run 33737705118
head d8ce0d41b133d36d6e0f79eac5a79c4266b05cfc
completed/success

Reference Legacy Import Check #132
run 33737704899
head d8ce0d41b133d36d6e0f79eac5a79c4266b05cfc
completed/success

ESP32 Build #1896
run 33737704907
head d8ce0d41b133d36d6e0f79eac5a79c4266b05cfc
completed/success

CMP Protocol Tests #4891
run 33737723762
head 162e94a9e7539236ff747720cc47850f4156e9f0
completed/success
```

## Regression coverage

`Tests/Web/check_hall_history_ui.js` теперь требует:

- `resultBox.textContent='Результат: ';`
- DOM `<b>` для validity;
- `document.createTextNode(...)` для runtime результата;
- отсутствие `resultBox.innerHTML=`.

Существующие Hall history/API/reference navigation contracts сохранены.

## NO-CHANGE findings

- `makeHistoryRow()` уже использует `textContent` для measurement, recommendation и persisted EEPROM fields;
- history errors также выводятся через `textContent`;
- Hall ARM/ABORT/APPLY API не менялись;
- ESP32 proposal не является physical START;
- EEPROM применение по-прежнему требует отдельного локального подтверждения `#` на Arduino.

## Safety invariants

Не изменены:

- physical START только на Arduino;
- Web не запускает SSR;
- no auto-resume;
- calibration run остаётся bounded;
- применение Hall settings отделено от движения;
- Arduino остаётся authoritative владельцем сохранённых Hall settings.

## NEXT

Продолжить final Web/runtime acceptance audit по оставшимся runtime DOM/URL boundaries. Исправлять только подтверждённые дефекты; безопасные области фиксировать как **NO-CHANGE**.
