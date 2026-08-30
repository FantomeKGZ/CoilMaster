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

Свежая independently verified documentation chain на `arduino-ru-lcd-experiment` подтверждена непрерывно through **CMP #4509**.

Последние подтверждённые runs:

```text
#4500  run 33312006146 / SUCCESS  head 7b0340de555b0a6e0bcb4034d5ba9f82375e2d95
#4501  run 33312028901 / SUCCESS  head 1cf362fb8500abeb546365f96b8a18ce59511bd9
#4502  run 33312118683 / SUCCESS  head 2d76a44b7c4a8f051cff4035ffbec9487265bde1
#4503  run 33312143177 / SUCCESS  head d4d962387ada7f97e217355d8b4b52b63e35be84
#4504  run 33312165284 / SUCCESS  head 1ba67a24f512eb22c92a261820c7495e6b2b9559
#4505  run 33312246571 / SUCCESS  head aa57043d7a57ce63850efc9280ffbf2eed65ec3b
#4506  run 33312267626 / SUCCESS  head 5fb842b9c89f35634e0dadbd48ee10930194eae9
#4507  run 33312294361 / SUCCESS  head c565431e0f5622e3d2c25f4824946bdff1dada42
#4508  run 33312375450 / SUCCESS  head 147713b1cb229276b9b880ccf25f4188b5f392bf
#4509  run 33312397960 / SUCCESS  head 6733c043cd7cb6068c9383ddc2af717c9662cad1
```

Полная непрерывная цепочка #4160–#4509 хранится в `16_CMP_4160_4162_GREEN_2026-08-30.md`.

Latest exact independently verified GREEN head before this documentation update:

```text
6733c043cd7cb6068c9383ddc2af717c9662cad1
CMP Protocol Tests #4509  run 33312397960 / SUCCESS
```

#4508 verifies snapshot through #4507 and #4509 verifies entrypoint through #4507. Transfer through #4507 (`cf8c6555d41248e452477b4d597539ab5d0f4e7b`) пока не подтверждён отдельным supplied exact SUCCESS. Documentation-only runs не заменяют firmware/build evidence checkpoints 166–167. Новые docs commits through #4509 новее latest exact GREEN и не должны называться GREEN без exact run evidence. Не создавать бесконечную цепочку docs commits только ради записи SUCCESS предыдущего docs commit.

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
