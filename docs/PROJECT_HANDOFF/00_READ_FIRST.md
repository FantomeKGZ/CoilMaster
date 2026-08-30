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

Свежая independently verified documentation chain на `arduino-ru-lcd-experiment` теперь подтверждена through **CMP #4185**:

```text
#4160  run 33296596573 / SUCCESS  head d8a597c1ac07ee234a824a419698a4dc61067761
#4161  run 33296706502 / SUCCESS  head 124464fc728414c6ba770669755a57e724e4710c
#4162  run 33296723545 / SUCCESS  head 4be7c2204eb2d552950b0e4648cb24358d6e356e
#4163  run 33296961701 / SUCCESS  head 8e451fb65495792cf69f9b96c4c43350835b622a
#4164  run 33296984838 / SUCCESS  head 159da0c7abb90dba8c6c00e5da40d835fa28d106
#4165  run 33297079657 / SUCCESS  head 18bca1973b2cf0f369970ea8cc84856f11aae630
#4166  run 33297104319 / SUCCESS  head c26c58729e94db03895ad49d42322360dd4d4afd
#4167  run 33297180420 / SUCCESS  head 702984933d795dff8a58c11cc15bf0bc68ac9547
#4168  run 33297201539 / SUCCESS  head 8b2442668a44ae9b115c856120a518a0bb2cb794
#4169  run 33297342101 / SUCCESS  head a9376388ad80376ca1190f41ef6ff203f4a08584
#4170  run 33297367289 / SUCCESS  head 914ad8e868a7908eb4386ee1f2bcc180f8ddf3a8
#4171  run 33297388482 / SUCCESS  head f8e7232009b1aa5e4900e7692256687d0be4704a
#4172  run 33297482944 / SUCCESS  head 87286e3380bb2ca01b1da9d30b3106a53c7d8413
#4173  run 33297519488 / SUCCESS  head aada5bf6eb3a7c8d17e24930638ab5f69dc9b81e
#4174  run 33297539871 / SUCCESS  head c96439dbcb5db73cc97f2e3f672222fbc72ab082
#4175  run 33297558840 / SUCCESS  head 3128287b0fcbe404a377a0ac16fc8987ea377e2f
#4176  run 33297636012 / SUCCESS  head b7d85d5fb1e478d6ce4d9aad75e855c1b5a12a85
#4177  run 33297656125 / SUCCESS  head 67e4ee7b22b28d7c7574c82d66af6383f5fbea2b
#4178  run 33297675300 / SUCCESS  head 93acd7112db32f81b1a1b3f70cdd5bd171cbb495
#4179  run 33301160832 / SUCCESS  head a47b788995c31c9edd1b31d5f7abeecab962ea82
#4180  run 33301185698 / SUCCESS  head 53176b964123f97eca65461bdfda5bb4b490c0c0
#4181  run 33301246334 / SUCCESS  head c4a85986e5b32ded3e7f596b2a6959e6187ce676
#4183  run 33301292351 / SUCCESS  head ade926838f966e5e7a01cda274f3187bae4e7d26
#4184  run 33301333347 / SUCCESS  head 9f712e53a4cf5ff1411b60069136736eedcca9a9
#4185  run 33301354570 / SUCCESS  head d502921a9a18994a630366ed305a20bbd9cf584a
```

`#4182` в текущей подтверждённой серии не зафиксирован и не должен предполагаться автоматически.

Latest exact independently verified GREEN head before this documentation update:

```text
d502921a9a18994a630366ed305a20bbd9cf584a
CMP Protocol Tests #4185  run 33301354570 / SUCCESS
```

Detailed CI snapshot: `16_CMP_4160_4162_GREEN_2026-08-30.md` (filename retained for stable reference; content is extended through #4185).

Documentation-only runs do not replace firmware/build evidence for checkpoints 166–167. Do not create an endless chain of documentation commits merely to record SUCCESS of the immediately preceding documentation commit.

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
