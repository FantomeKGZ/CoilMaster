# CoilMaster — current project entrypoint

Дата обновления: **2026-08-30**  
Repo: `FantomeKGZ/CoilMaster`  
Production/source-of-truth: **`cmp-protocol-v1`**. `main` для исходников не использовать.  
Текущая рабочая ветка: **`arduino-ru-lcd-experiment`**.

## Branch policy

Production остаётся неизменённым:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

Все новые изменения выполнять только в `arduino-ru-lcd-experiment`. Не переносить experiment обратно в `cmp-protocol-v1` без отдельного прямого запроса пользователя.

Перед каждым изменением существующего файла обязательно fetch current branch content и использовать current blob SHA. Для нового файла сначала подтвердить отсутствие пути.

## Latest independently verified experiment CI

Свежая independently verified documentation chain на `arduino-ru-lcd-experiment` подтверждена непрерывно through **CMP #4299**.

Последние подтверждённые runs:

```text
#4290  run 33304591751 / SUCCESS  head f5798845043f93ff779ce4af1235e8f58329718f
#4291  run 33304608041 / SUCCESS  head 573d9ce3f881fe90a7bb2e6ccc563c3a0c2d257f
#4292  run 33304661338 / SUCCESS  head 3fd463d1e4554c0c4a4bd43df7097a6884207111
#4293  run 33304676430 / SUCCESS  head 554946f5569cfd227e1197aaf76a39b6e1f69bba
#4294  run 33304692688 / SUCCESS  head 3c8a562bb5b5b8fc3c7e71ef03cf7b2351e277d9
#4295  run 33304737838 / SUCCESS  head bcf5635bb14ea416451ec8c5c044868653270c6b
#4296  run 33304751999 / SUCCESS  head 871184c6fb1424643acace01ae2d9ac15eba057a
#4297  run 33304771078 / SUCCESS  head 2445cf2be99f8315557f6e6505157e9517434017
#4298  run 33304820446 / SUCCESS  head 10b9f0f8da2a5e8b3e972599156f386e2904ba69
#4299  run 33304838485 / SUCCESS  head c2511a5a692794fd25c2f0834755aefa75eb0aaa
```

Полная непрерывная цепочка #4160–#4299 хранится в `16_CMP_4160_4162_GREEN_2026-08-30.md`.

Latest exact independently verified GREEN head before this documentation update:

```text
c2511a5a692794fd25c2f0834755aefa75eb0aaa
CMP Protocol Tests #4299  run 33304838485 / SUCCESS
```

Documentation-only runs не заменяют firmware/build evidence checkpoints 166–167. Не создавать бесконечную цепочку docs commits только ради записи SUCCESS предыдущего docs commit.

## Current engineering state

Experiment-side repo-reviewable software work закрыт through checkpoint **167**:

```text
152 RUN_WIRE Material Request status batching
153 unified autonomous/Web completed-job archive lifecycle
154 RUN_WIRE exact immutable-spool lookup
155 Material Request create repair scan reuse
156 Material Request Warehouse known-request status reuse
157 client balance repair-journal validation reuse
158 RepairCostingWeb exact repair proof reuse
159 autonomous winding -> canonical motor history projection
160 Warehouse exact lookup optimization
161 Warehouse CONFIRMED provenance suffix scan
162 repair finalization known-repair proof reuse
163 Repair Delivery single-pass append preflight
164 spool/material bridge suffix uniqueness audit
165 residual repeated-scan audit -> NO-CHANGE
166 reachable Hall RU LCD localization -> GREEN
167 static canonical winding-role selector cleanup -> GREEN
```

Repeated-scan/performance optimization считается исчерпанной до появления concrete measured bottleneck или defect.

## Physical Arduino + ESP32 E2E

На **2026-08-30** пользователь подтвердил физический тест реального CoilMaster: **всё работает нормально**.

Это operator-confirmed hardware evidence. Physical Arduino+ESP32 acceptance gate считается закрытым для текущего проверенного hardware/firmware состояния. Подробности: `13_HALL_RU_LCD_ACCEPTANCE.md` и `15_NEXT_CHAT_TRANSFER_2026-08-30.md`.

## Firmware/build evidence

Checkpoint 166 Hall RU LCD:

```text
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c
CMP Protocol Tests #4028  run 33268897356 / SUCCESS
Arduino RU LCD #206       run 33268897370 / SUCCESS
```

Uno resource evidence:

```text
uno_ru_lcd: RAM 1614 / 2048 (78.8%); Flash 31448 / 32256 (97.5%); headroom 808 bytes
uno:        RAM 1605 / 2048 (78.4%); Flash 31066 / 32256 (96.3%); headroom 1190 bytes
```

Checkpoint 167 canonical winding-role selector:

```text
9e538828ed179700d362286a3af72de6a6ce0b6f
CMP Protocol Tests #4068  / SUCCESS
ESP32 Build #1778         / SUCCESS
Arduino RU LCD #207       / SUCCESS

47903b0f2e2ddc8ac90abf1e26db7e678a570363
CMP Protocol Tests #4069  / SUCCESS
ESP32 Build #1779         / SUCCESS
Arduino RU LCD #208       / SUCCESS

0eb32376de3a4c50c765dcbe6b946524d075f69b
CMP Protocol Tests #4070  / SUCCESS
```

Практический вывод: не расширять Uno крупными Hall/UI-функциями; новые Uno-side изменения только для concrete defect и максимально малы.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md
docs/PROJECT_HANDOFF/10_CHECKPOINT_161_WAREHOUSE_PROVENANCE_SUFFIX_SCAN.md
docs/PROJECT_HANDOFF/11_CHECKPOINT_162_REPAIR_FINALIZATION_KNOWN_REPAIR.md
docs/PROJECT_HANDOFF/12_CHECKPOINT_163_165_REPEATED_SCAN_CLOSEOUT.md
docs/PROJECT_HANDOFF/13_HALL_RU_LCD_ACCEPTANCE.md
docs/PROJECT_HANDOFF/14_NEXT_CHAT_TRANSFER_2026-08-30.md
docs/PROJECT_HANDOFF/15_NEXT_CHAT_TRANSFER_2026-08-30.md
docs/PROJECT_HANDOFF/16_CMP_4160_4162_GREEN_2026-08-30.md
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```

## Immediate NEXT

1. Получить свежий HEAD `arduino-ru-lcd-experiment` перед любым изменением.
2. Treat checkpoints 159–167 as closed unless concrete regression is observed.
3. Physical Arduino+ESP32 E2E также закрыт operator-confirmed PASS; не переоткрывать без concrete hardware regression/new finding.
4. Не продолжать speculative repeated-scan refactors; checkpoint 165 = NO-CHANGE.
5. Дальнейшая работа только от concrete runtime/software defect, hardware finding, measured bottleneck или явно выбранной новой product/UX feature.
6. Не утверждать GREEN без exact CI/run evidence для соответствующего SHA.
7. Production `cmp-protocol-v1` не трогать без отдельного запроса.
