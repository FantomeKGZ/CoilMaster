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

Свежая independently verified documentation chain на `arduino-ru-lcd-experiment` подтверждена непрерывно through **CMP #4254**.

Последние подтверждённые runs:

```text
#4245  run 33303344750 / SUCCESS  head 40466baf0001cb2fb525159a106892b184e704d9
#4246  run 33303362426 / SUCCESS  head 181c55599d5d3642238509cc10353739fe03f365
#4247  run 33303424715 / SUCCESS  head 3247e9f793feb06d4c4af8f1a728b186e09aaf2c
#4248  run 33303438829 / SUCCESS  head f940d84f07d4ccddcb4603c22b11ca06be3d1357
#4249  run 33303460375 / SUCCESS  head 9b8377115a65dedfdcefd395b1ef3ec01d351dcf
#4250  run 33303512810 / SUCCESS  head 3915771cb573ce91f6e3bc0c84e54fd0a7be7457
#4251  run 33303527840 / SUCCESS  head 611ec075f175c66832dbc08e852c72178d992584
#4252  run 33303547940 / SUCCESS  head 3bbf453af708e89e4afc1be5e8c88a0f3b9e7ca0
#4253  run 33303594288 / SUCCESS  head 359c4565a18b3ba0ca399ddb9e9305ea6b80d0cb
#4254  run 33303613275 / SUCCESS  head 31a2a00f325238a898dc2e3943f4f57c525210a1
```

Полная непрерывная цепочка #4160–#4254 хранится в `16_CMP_4160_4162_GREEN_2026-08-30.md`.

Latest exact independently verified GREEN head before this documentation update:

```text
31a2a00f325238a898dc2e3943f4f57c525210a1
CMP Protocol Tests #4254  run 33303613275 / SUCCESS
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
