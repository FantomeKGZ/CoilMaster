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

Все новые изменения текущего цикла выполнять только в `arduino-ru-lcd-experiment`. Не переносить experiment обратно в `cmp-protocol-v1` без отдельного прямого запроса пользователя.

Перед каждым изменением существующего файла обязательно fetch current branch content и использовать current blob SHA. Для нового файла сначала подтвердить отсутствие пути.

## Latest independently verified experiment CI

GitHub metadata independently confirms the latest documentation chain:

```text
8b3926b8de870589b64f0e0107d2f7a099e89c70
CMP Protocol Tests #4134  run 33295091259 / SUCCESS
message: docs(handoff): advance entrypoint through CMP 4131

b97fd6b9f77496646573bbd1ea64c151c049a78f
CMP Protocol Tests #4133  run 33295068299 / SUCCESS
message: docs(handoff): record CMP 4131

103dc4ef9267c266ae64acadbe2dd198d6a77eed
CMP Protocol Tests #4132  run 33294968670 / SUCCESS
message: docs(handoff): extend verified CI chain through 4130

58dda8de4b76861d05a14390bc3760f4647b7876
CMP Protocol Tests #4131  run 33294943179 / SUCCESS

fd098e0bbd72acc2c7e5c11b397e2315314343d5
CMP Protocol Tests #4130  run 33294860227 / SUCCESS

2390d6e1916c2bd2cbbcb72901cf486e200c72e2
CMP Protocol Tests #4129  run 33294838984 / SUCCESS

f833937bdea98c38a200cce4f297a95d62513d80
CMP Protocol Tests #4128  run 33294756834 / SUCCESS
```

For `#4132/#4133/#4134`, branch is `arduino-ru-lcd-experiment`, event is `push`, status is `completed`, conclusion is `success`, and exact heads are `103dc4ef9267c266ae64acadbe2dd198d6a77eed`, `b97fd6b9f77496646573bbd1ea64c151c049a78f`, and `8b3926b8de870589b64f0e0107d2f7a099e89c70`.

Therefore `8b3926b8de870589b64f0e0107d2f7a099e89c70` is an exact verified GREEN documentation head. Any documentation commit after it must get its own exact CI result before being called GREEN.

The longer verified handoff chain through `#4119` remains recorded in `14_NEXT_CHAT_TRANSFER_2026-08-30.md` and Git history. Documentation-only runs do not replace separate firmware/build evidence checkpoints 166–167.

Stable pre-CRM snapshot remains:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
stable-2026-08-25-pre-crm-redesign -> same commit
```

## Read order

Для нового чата сначала читать:

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
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```

Старые numbered checkpoint-файлы остаются историей, но не являются текущей точкой входа.

## Current state

Experiment-side repo-reviewable software work закрыт through checkpoint **167**.

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

Repeated-scan/performance optimization считается исчерпанной до появления конкретного измеренного bottleneck или дефекта. Не продолжать speculative storage refactors только ради уменьшения количества file opens.

## Checkpoint 166 — Hall RU LCD firmware evidence

```text
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c
CMP Protocol Tests #4028  run 33268897356 / SUCCESS
Arduino RU LCD #206       run 33268897370 / SUCCESS
```

Exact #206 Uno resource evidence:

```text
uno_ru_lcd: RAM 1614 / 2048 (78.8%); Flash 31448 / 32256 (97.5%); headroom 808 bytes
uno:        RAM 1605 / 2048 (78.4%); Flash 31066 / 32256 (96.3%); headroom 1190 bytes
```

Практический вывод: не расширять Uno крупными Hall/UI-функциями. Новые Arduino-side изменения только для конкретных дефектов и максимально малы; расширенную обработку/представление по возможности держать на ESP32, сохраняя автономную безопасность Arduino.

## Checkpoint 167 — canonical winding-role selector evidence

```text
9e538828ed179700d362286a3af72de6a6ce0b6f
CMP Protocol Tests #4068  run 33290408963 / SUCCESS
ESP32 Build #1778         run 33290408891 / SUCCESS
Arduino RU LCD #207       run 33290408886 / SUCCESS

47903b0f2e2ddc8ac90abf1e26db7e678a570363
CMP Protocol Tests #4069  run 33290422893 / SUCCESS
ESP32 Build #1779         run 33290422888 / SUCCESS
Arduino RU LCD #208       run 33290422860 / SUCCESS

0eb32376de3a4c50c765dcbe6b946524d075f69b
CMP Protocol Tests #4070  run 33290440543 / SUCCESS
```

Static desktop/mobile selectors expose only canonical `WORKING` / `STARTING`. Runtime stale-page filtering and backend unsupported-role rejection remain defense-in-depth; occupied-role replacement remains explicit and never auto-retried.

## Immediate NEXT

1. Получить свежий HEAD `arduino-ru-lcd-experiment` перед любым изменением.
2. Treat checkpoints 159–167 as closed unless a concrete regression is observed.
3. Не продолжать speculative repeated-scan refactors; checkpoint 165 is NO-CHANGE.
4. Следующая repo-only работа должна идти только от конкретного дефекта, measured bottleneck либо hardware-acceptance находки.
5. Full Arduino + ESP32 hardware E2E остаётся обязательным финальным acceptance gate.
6. Во время hardware E2E проверить UART command/ack, keypad, normal RU screens before/after Hall, physical START ownership, Hall 15-second run/apply/reject, SSR fail-safe, RUN evidence, manual exact RUN_WIRE writeoff и reboot/recovery fail-closed behavior.
7. Перед каждым изменением существующего файла fetch current branch content + current blob SHA.
8. Не утверждать GREEN без exact current CI/run evidence.
9. Production `cmp-protocol-v1` не трогать без отдельного запроса.
