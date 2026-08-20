# CoilMaster — UI, hardware settings and JOB lifecycle plan

Дата: **2026-08-20**  
Ветка: **`cmp-protocol-v1`**  
Статус: **утверждённый план следующего большого блока; код реализовывать по этапам, сохраняя safety-инварианты**

## Source of truth

Использовать только `FantomeKGZ/CoilMaster`, ветку `cmp-protocol-v1`. `main` не использовать как источник реализации.
Перед изменением существующего файла всегда заново fetch текущую версию из `cmp-protocol-v1` и использовать актуальный blob SHA.

## Safety-инварианты — не менять

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` сам по себе не выполняет automatic wire writeoff;
- wire writeoff остаётся ручным и должен сохранять exact provenance;
- hardware settings нельзя менять во время активной/неопределённой намотки;
- web может настраивать только безопасные параметры, но не выполнять direct START/SSR ON;
- история RUN остаётся immutable; linkage к motor не переписывает физические факты.

---

# A. Обязательный функциональный блок: модель двигателя и UI

Первые требования пользователя считаются основными и обязательными.

## A1. Форма двигателя

Новая понятная модель должна включать минимум:

- `manufacturer` — Производитель;
- `model` — Модель;
- `phase_count` — количество фаз (минимум 1/3, schema допускает расширение);
- `slot_count` — количество пазов;
- `coil_program` / `program[]` — программа витков, например `38/38`;
- `repeat_target` — сколько раз выполнить всю программу, например `6`;
- отдельные расширенные поля: мощность, напряжение, скорость, полюса, шаг, провод, параллельные жилы, соединение, комментарий, теги;
- физический `coil_count`, если понадобится как справочная характеристика, хранить отдельно и НЕ вычислять из `38/38 × 6`.

Поле `name` переосмыслить: по умолчанию интерфейс должен строить понятный заголовок из `manufacturer + model`; legacy `name` сохранить для backward compatibility/import, но не делать главным обязательным полем для оператора.

## A2. Семантика программы и повторов

Критично:

```text
38/38 × 6
```

означает:

```text
program = [38, 38]
repeat_target = 6
```

ESP32 отправляет Arduino **один JOB** с одной программой и `repeat_target=6`.
Arduino после каждого завершённого выполнения предлагает оператору следующий повтор и ждёт физический START.
Нельзя превращать это в 12 отдельных катушек или 6 отдельных JOB.

Нужно различать:

- `repeat_target` — сколько запланировано;
- `completed_runs` — сколько реально выполнено в текущем JOB;
- `historical_runs` — сколько раз программа выполнялась когда-либо по архиву;
- `coil_count` — независимая физическая характеристика, если требуется.

## A3. Список двигателей

Desktop quick list минимум:

```text
Производитель | Модель | Фазы | Пазы | Программа | Повторы | Последняя работа
```

ID показывать вторично.
Добавить быстрые фильтры по manufacturer/model/phase_count/slot_count/program и поиск.

## A4. Detail page двигателя

Создать отдельные detail pages desktop/mobile.
Карточка должна содержать:

- manufacturer/model;
- phase_count;
- slot_count;
- program;
- repeat_target;
- расширенные технические параметры;
- соединение и будущую картинку схемы;
- вес одной катушки/обмоточной части, если оператор его задаёт;
- историю ремонтов/намоток;
- фактический исторический расход меди/алюминия в кг;
- будущие фото шильдика/статора/соединения — хранить вне `/web`.

Из карточки нужны действия `Новый ремонт` и просмотр истории.

---

# B. Обязательный функциональный блок: автономные намотки Arduino

## B1. Новый список вместо карточек

`desktop/arduino-windings.html` и mobile аналог переделать в компактный список/таблицу.
Минимальные поля:

- checkbox;
- session/run ID;
- программа;
- количество завершений/выполнений;
- planned repeat target, если запись относится к JOB;
- статус;
- linkage к motor;
- сколько раз такая программа встречалась в истории.

Длинные тексты статусов заменить badges/icons.
На desktop tooltip должен работать по hover и keyboard focus; на mobile — доступное раскрытие/описание.

## B2. Bulk actions

Сверху списка:

- создать новый двигатель из выбранных записей;
- привязать выбранные записи к существующему двигателю;
- объединить несколько выбранных программ в один новый двигатель;
- убрать дублирующиеся формы создания двигателя из каждой карточки.

Linkage — отдельная операция. Исходные `RUN_STARTED/RUN_COMPLETED/recovered` события не изменять и не удалять.

---

# C. Обязательный функциональный блок: расход провода в кг

Пользователь ведёт основной фактический расход в кг и не хочет обязательной жёсткой привязки каждого расхода к конкретной бухте.

Нужно переработать модель осторожно:

- kg remains authoritative operator-entered quantity;
- допускается writeoff без обязательного operational spool selection;
- сохранить audit provenance `source_session_id + source_run_id + material/conductor identity`;
- `spool_id` сделать optional/nullable для нового режима, не уничтожая legacy records;
- если spool указан, сохранять его как дополнительную provenance/warehouse информацию;
- миграция только backward-compatible, без переписывания append-only истории;
- finalization должен требовать подтверждённый manual writeoff для завершённых linked runs, но не обязательно exact spool, если новая схема уже активирована и record schema это явно обозначает.

Перед реализацией требуется отдельный storage/API audit всех spool invariants.

---

# D. Web UX core

## D1. Общий app shell

Сейчас desktop/mobile страницы имеют несогласованные меню. Создать shared shell/navigation layer:

- одинаковый sidebar/topbar desktop;
- единая mobile navigation;
- active section;
- переключатель desktop/mobile;
- clock area;
- firmware/web version area;
- common status/toast/error handling.

`desktop/settings-ftp.html` должен получить обычную верхнюю и боковую навигацию.

## D2. Время на всех страницах

Использовать RTC/Asia/Bishkek как источник device time.
Не делать отдельный тяжёлый request каждую секунду.
Предпочтительно:

1. при загрузке shell получить device timestamp + sync/source state;
2. дальше локально увеличивать время в browser каждую секунду;
3. редкий re-sync 30–60 секунд или при visibility restore/network reconnect;
4. показывать source badge: `RTC`, `NTP synced`, `time unavailable`.

## D3. Дополнительные UX улучшения

После основного блока:

- глобальный поиск motor/client/repair;
- recent items;
- breadcrumbs;
- отдельная detail page ремонта;
- единый error map;
- toast notifications вместо `alert()`;
- unsaved-form guard;
- structured fields вместо технических значений в tags;
- similarity reasons для дубликатов motors;
- grouped Arduino archive view по программе;
- stale-data indicator + last updated time;
- diagnostics page;
- UI contract test: shell/nav/clock/internal links/desktop-mobile pair.

---

# E. Hardware settings: Hall и другие устройства

## E1. Старый Hall baseline

Рабочий старый скетч пользователя задавал:

```text
Hall pin: A0
idle about: 522
magnet about: 660
threshold: 590
hysteresis/release delta: 50
release threshold: 540
```

Текущий проект уже использует defaults:

```text
HallThreshold = 590
HallHysteresis = 50
```

Это оставить factory/default baseline.

## E2. Найденная Hall bug и текущий hardening

На реальном стенде магнит, оставшийся около датчика, мог вызывать повторный счёт из-за шума/колебаний ADC вокруг release threshold.

В `CM_HallTurnSource` добавлена защита: следующий импульс должен re-arm только после стабильного ухода сигнала ниже release threshold в течение заданного времени. Базовые 590/50 не меняются.

Последние изменения:

```text
081b3ed1dc3849ec8b0c6898fd841acb6d5f2d76
fix: debounce Hall sensor release before rearming

2b14a91c5a90d8fd2d694c5c54308f47fcfea0b4
fix: require stable Hall release before next turn
```

Нужен Arduino hardware regression. Build/CI не считать подтверждёнными без фактического результата.

## E3. Live Hall calibration — обязательный UX

Создать `Настройки → Оборудование → Датчик Холла`.

Показывать live:

- `raw_adc` текущий;
- min ADC за короткое окно;
- max ADC за короткое окно;
- текущий threshold;
- hysteresis;
- вычисленный release threshold;
- release debounce ms;
- `magnet_detected`;
- состояние re-arm: `ARMED / BLOCKED_WAIT_RELEASE / RELEASE_DEBOUNCE`;
- timestamp/age последнего sample.

UI действия:

- `Зафиксировать покой` — собрать bounded sample window без магнита;
- `Зафиксировать магнит` — собрать bounded sample window с магнитом;
- показать measured ranges, noise span и предложенный threshold;
- оператор вручную подтверждает значения;
- `Вернуть стандартные 590/50`;
- `Сохранить в Arduino` только в safe idle.

Предложенный threshold не должен сохраняться автоматически.

### Рекомендуемый алгоритм предложения

Для текущей rising-polarity установки:

```text
idle_high < magnet_low
recommended_threshold ≈ midpoint(idle_high, magnet_low)
```

Нужен safety margin от обоих диапазонов.
Если диапазоны перекрываются, UI показывает `Калибровка ненадёжна` и не предлагает автоматическое значение.

В будущем можно добавить `Hall direction: RISING/FALLING`, но сначала подтвердить hardware need; текущий стандарт соответствует старому SS49E setup пользователя, где магнит повышал ADC.

## E4. Частота live данных

Не засорять UART и ESP32 polling.
Рекомендуемый режим:

- telemetry stream включается только когда calibration page открыта;
- Arduino sample можно читать часто локально, но передавать агрегированно 5–10 Hz;
- ESP32 держит последний sample/cache;
- browser polling 250–500 ms или компактный event stream, если это дешевле текущей архитектуры;
- при закрытии страницы telemetry отключается;
- отсутствие telemetry никак не влияет на обычный winding loop.

## E5. Persistence hardware settings

Hall settings должны храниться на Arduino в EEPROM с:

- schema/version;
- CRC;
- default fallback;
- atomic/dual-slot или иной recoverable commit pattern;
- bounded validation ranges;
- readback verify после save.

При invalid EEPROM Arduino запускается с безопасными factory defaults 590/50 и явно сообщает состояние ESP32.

## E6. Safe settings protocol

Добавить отдельные CMP messages, naming уточнить при реализации, например:

```text
CFG_GET
CFG_STATE
CFG_SET|HALL_THRESHOLD|590
CFG_SET|HALL_HYSTERESIS|50
CFG_SET|HALL_RELEASE_DEBOUNCE_MS|25
CFG_ACK
CFG_NACK
```

`CFG_SET` разрешать только при доказанном safe idle и никогда не связывать с START/SSR.

## E7. Другие доступные настройки

Операторские:

- Hall threshold/hysteresis/release debounce;
- buzzer enabled/volume-pattern only if hardware supports it;
- LCD backlight;
- START debounce.

Read-only/service by default:

- pins;
- UART baud;
- SSR polarity/control;
- hardware feature flags.

Web не получает direct SSR test that energizes the motor. Допускается только безопасная логическая диагностика без физического START.

---

# F. JOB lifecycle — устранение задания, которое остаётся навсегда

## F1. Подтверждённая причина

Текущий Arduino `StateMachine` после `RUN_COMPLETED` переводит состояние в `JobComplete`, но сохраняет `m_job`.
`startOrResume()` из `JobComplete` специально позволяет снова начать тот же job.
Поэтому remote job может оставаться бесконечно, пока оператор явно не очистит его.

## F2. Целевая семантика с repeat_target

Один remote JOB:

```text
program = 38/38
repeat_target = 6
```

Каждый физический повтор:

```text
physical START
→ RUN_STARTED(run N)
→ winding
→ RUN_COMPLETED(run N)
→ completed_runs++
```

Если `completed_runs < repeat_target`:

```text
WAITING_REPEAT_START
```

Arduino показывает `Выполнено N/6` и ждёт физический START.

Если `completed_runs == repeat_target`:

```text
FINAL_COMPLETED_PENDING_DELIVERY
```

После надёжного сохранения completion и подтверждения доставки/согласованного finalization handshake active job очищается и Arduino возвращается HOME.
История RUN остаётся.

## F3. Нельзя очищать слишком рано

Не очищать active job в тот же момент, когда достигнут последний виток, до persistence/event handoff.
Нужно сохранить возможность replay `RUN_COMPLETED` после потери связи/ESP32 reboot.

Цель:

```text
physical completion
→ SSR OFF
→ persist RUN_COMPLETED evidence
→ deliver/replay to ESP32
→ ACK/DUPLICATE accepted
→ final job clear
→ HOME
```

## F4. Early finish

Если оператор закончил раньше плановых повторов:

```text
repeat_target = 6
completed_runs = 4
completion_reason = OPERATOR_FINISHED_EARLY
```

Не удалять четыре RUN и не превращать их в обычный CANCELLED без объяснения.

## F5. Recovery invariants

- no auto-resume after reboot;
- persisted pending completion replay continues;
- `ALL_CLEAR`/JOB_CANCEL behavior для no-run uncertainty сохраняется;
- active/paused run cancel fail-closed;
- final clear не создаёт дополнительный `RUN_COMPLETED`;
- wire writeoff manual only.

---

# G. Firmware/Web identification

Продолжить уже начатый build identity block:

- ESP32 firmware Git SHA/branch/build time в Serial и API;
- web build/version рядом с firmware build;
- diagnostics показывает несовпадение firmware/web;
- это позволяет отличать обновление `/web` от реальной перепрошивки ESP32.

До утверждения build-success сверять Actions/реальный build, а не только наличие кода.

---

# H. План реализации по этапам

## Phase 0 — reconcile current branch

1. fetch current HEAD `cmp-protocol-v1`;
2. проверить завершение/состояние firmware identity workflow;
3. clean build `uno` и `esp32`;
4. protocol/web tests;
5. зафиксировать только подтверждённые результаты.

## Phase 1 — Hall correctness first

1. завершить/проверить stable-release protection;
2. добавить unit/host test abstraction, если возможно без `analogRead` hard dependency;
3. hardware regression: stationary magnet must produce max one turn until true release;
4. проверить реальный ADC idle/magnet/noise;
5. подобрать debounce bounds по измерениям.

Acceptance:

```text
stationary magnet -> one count max
remove + return magnet -> exactly next count
magnet present at START -> no false repeated counts
noise around release threshold -> no repeated count
```

## Phase 2 — Arduino hardware settings persistence + protocol

1. EEPROM schema + CRC/recovery;
2. Hall read/write settings;
3. safe CFG protocol;
4. live telemetry state;
5. UNO build/tests.

## Phase 3 — Hardware settings UI

1. desktop/mobile equipment page;
2. live ADC/current/min/max/status;
3. capture idle/magnet windows;
4. suggested threshold + manual confirm;
5. restore defaults;
6. safe idle blocking and clear error messages.

## Phase 4 — JOB repeat_target and automatic final clear

1. extend shared WindingJob/protocol schema backward-compatibly;
2. state-machine states for repeat wait/final delivery;
3. preserve physical START requirement;
4. persist/replay final completion;
5. clear active job after final acknowledged completion;
6. early-finish reason;
7. protocol tests + UNO/ESP32 builds;
8. targeted hardware E2E.

## Phase 5 — Motor schema migration

1. add phase_count/slot_count/repeat_target;
2. legacy records parse with nullable fields;
3. imports/similarity/search updated;
4. no speculative values for old records;
5. detail APIs as needed.

## Phase 6 — Motors desktop/mobile redesign

1. new form;
2. structured quick list;
3. filters;
4. detail page;
5. history/material usage;
6. future diagram/photo slots.

## Phase 7 — Arduino archive redesign

1. compact list;
2. status icons/tooltips;
3. multi-select;
4. link selected to existing motor;
5. create motor from selected;
6. grouped program summary;
7. immutable provenance retained.

## Phase 8 — kg-first material/writeoff migration

1. audit spool coupling;
2. define schema version and optional spool provenance;
3. migrate API/finalization safely;
4. preserve append-only history;
5. hardware/business flow tests.

## Phase 9 — Shared web shell and additional UX

1. unified desktop/mobile navigation;
2. fix FTP page shell;
3. global clock;
4. firmware/web version;
5. shared toasts/errors;
6. global search/recent/breadcrumbs;
7. UI contract tests.

---

# I. Verification matrix for this program of work

At minimum after relevant phases:

```text
pio run -e uno
pio run -e esp32
node Tests/Web/check_web_assets.js
node Tests/Web/check_release_contracts.js
node Tests/Web/check_final_acceptance_contracts.js
```

Add/change-scoped host tests for protocol/state-machine/settings persistence.

Hardware regression must include:

```text
Hall stationary magnet behavior
Hall calibration live values
Hall settings survive Arduino reboot
invalid settings EEPROM -> factory defaults + reported warning
JOB 38/38 x6 -> one JOB, six physical STARTs max
no automatic START between repeats
final repeat -> event persisted/delivered -> active JOB auto-clears
ESP32 reboot during pending final completion -> replay, no auto-resume
operator early finish preserves completed RUN history
FTP AP/STA after actual firmware identification
RTC/NTP Asia/Bishkek
manual wire writeoff remains manual
```

---

# J. Точка продолжения для нового чата

В новом чате сказать:

```text
Продолжаем CoilMaster.
Repo FantomeKGZ/CoilMaster, source-of-truth branch cmp-protocol-v1, main не использовать.
Сначала прочитай docs/PROJECT_HANDOFF/00_READ_FIRST.md и
40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md.
Перед изменением существующего файла fetch актуальный blob SHA.
Продолжай сразу кодом/коммитами, но не утверждай build/CI green без фактической проверки.
Начать с Phase 0/Phase 1: reconcile branch и Hall hardware correctness, затем hardware settings live calibration.
```

Этот checkpoint является основной точкой продолжения для нового блока UI + hardware settings + repeat/JOB lifecycle после checkpoint 39.
