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

Свежая exact documentation chain на `arduino-ru-lcd-experiment`:

```text
a3082500e9295ee38823456ff69c8b6530b369da
CMP Protocol Tests #4154  run 33296258713 / SUCCESS
message: docs(handoff): checkpoint exact CMP 4153 state

f99bdcf3f108243e3192c8e83a66b289469681ed
CMP Protocol Tests #4155  run 33296340509 / SUCCESS
message: docs(handoff): confirm CMP 4154 after 4153 checkpoint

00919cbf5e8cd847a0e622bdbfe7bf4b291ab7f5
CMP Protocol Tests #4156  run 33296357807 / SUCCESS
message: docs(handoff): advance entrypoint through CMP 4154

dc9ff401cbb2e5dc68b0311c6a28f142462d7cab
CMP Protocol Tests #4157  run 33296440587 / SUCCESS
message: docs(handoff): record CMP 4155 and 4156

945061512a705a5b4f61a054841c977b7e978c9e
CMP Protocol Tests #4158  run 33296459453 / SUCCESS
message: docs(handoff): advance through CMP 4156

bd7e1f8454a25305fc0a4af47361341ba161d84f
CMP Protocol Tests #4159  run 33296578402 / SUCCESS
message: docs(handoff): record CMP 4157 and 4158

d8a597c1ac07ee234a824a419698a4dc61067761
CMP Protocol Tests #4160  run 33296596573 / SUCCESS
message: docs(handoff): advance through CMP 4158

124464fc728414c6ba770669755a57e724e4710c
CMP Protocol Tests #4161  run 33296706502 / SUCCESS
message: docs(handoff): record CMP 4159

4be7c2204eb2d552950b0e4648cb24358d6e356e
CMP Protocol Tests #4162  run 33296723545 / SUCCESS
message: docs(handoff): advance transfer through CMP 4159

8e451fb65495792cf69f9b96c4c43350835b622a
CMP Protocol Tests #4163  run 33296961701 / SUCCESS
message: docs(handoff): record CMP 4160-4162

159da0c7abb90dba8c6c00e5da40d835fa28d106
CMP Protocol Tests #4164  run 33296984838 / SUCCESS
message: docs(handoff): advance entrypoint through CMP 4162

18bca1973b2cf0f369970ea8cc84856f11aae630
CMP Protocol Tests #4165  run 33297079657 / SUCCESS
message: docs(handoff): extend CMP snapshot through 4164

c26c58729e94db03895ad49d42322360dd4d4afd
CMP Protocol Tests #4166  run 33297104319 / SUCCESS
message: docs(handoff): advance entrypoint through CMP 4164

702984933d795dff8a58c11cc15bf0bc68ac9547
CMP Protocol Tests #4167  run 33297180420 / SUCCESS
message: docs(handoff): record CMP 4165 and 4166

8b2442668a44ae9b115c856120a518a0bb2cb794
CMP Protocol Tests #4168  run 33297201539 / SUCCESS
message: docs(handoff): advance entrypoint through CMP 4166
```

Для #4154–#4168 GitHub metadata подтверждает branch = `arduino-ru-lcd-experiment`, event = `push`, status = `completed`, conclusion = `success`. Для #4160–#4162 и #4166–#4168 дополнительно проверен `host-tests`: configure/build/test и все audit steps завершены `success`. #4163–#4168 independently verified exact run metadata также `completed/success`.

Latest exact independently verified GREEN documentation head перед текущими documentation-only updates:

```text
8b2442668a44ae9b115c856120a518a0bb2cb794
CMP Protocol Tests #4168  run 33297201539 / SUCCESS
```

Текущие documentation-only updates после `8b244266...` нельзя называть GREEN, пока для их exact HEAD не будет собственного SUCCESS.

Подробный актуальный handoff находится в `15_NEXT_CHAT_TRANSFER_2026-08-30.md`; exact #4160–#4168 snapshot — в `16_CMP_4160_4162_GREEN_2026-08-30.md` (имя файла сохранено для стабильной ссылки, заголовок/содержимое расширены through #4168). Documentation-only runs не заменяют firmware/build evidence checkpoints 166–167.

Stable pre-CRM snapshot:

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
docs/PROJECT_HANDOFF/15_NEXT_CHAT_TRANSFER_2026-08-30.md
docs/PROJECT_HANDOFF/16_CMP_4160_4162_GREEN_2026-08-30.md
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```

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

Repeated-scan/performance optimization считается исчерпанной до появления конкретного measured bottleneck или дефекта. Не продолжать speculative storage refactors только ради уменьшения количества file opens.

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
2. Treat checkpoints 159–167 as closed unless concrete regression is observed.
3. Не продолжать speculative repeated-scan refactors; checkpoint 165 = NO-CHANGE.
4. Следующая repo-only работа — только concrete defect, measured bottleneck либо hardware-acceptance finding.
5. Full Arduino + ESP32 hardware E2E остаётся обязательным финальным acceptance gate.
6. Во время hardware E2E проверить UART command/ack, keypad, normal RU screens before/after Hall, physical START ownership, Hall 15-second run/apply/reject, SSR fail-safe, RUN evidence, manual exact RUN_WIRE writeoff и reboot/recovery fail-closed behavior.
7. Не утверждать GREEN без exact current CI/run evidence.
8. Production `cmp-protocol-v1` не трогать без отдельного запроса.