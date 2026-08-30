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
CMP Protocol Tests #4160  run 33296596573 / SUCCESS  head d8a597c1ac07ee234a824a419698a4dc61067761
CMP Protocol Tests #4161  run 33296706502 / SUCCESS  head 124464fc728414c6ba770669755a57e724e4710c
CMP Protocol Tests #4162  run 33296723545 / SUCCESS  head 4be7c2204eb2d552950b0e4648cb24358d6e356e
CMP Protocol Tests #4163  run 33296961701 / SUCCESS  head 8e451fb65495792cf69f9b96c4c43350835b622a
CMP Protocol Tests #4164  run 33296984838 / SUCCESS  head 159da0c7abb90dba8c6c00e5da40d835fa28d106
CMP Protocol Tests #4165  run 33297079657 / SUCCESS  head 18bca1973b2cf0f369970ea8cc84856f11aae630
CMP Protocol Tests #4166  run 33297104319 / SUCCESS  head c26c58729e94db03895ad49d42322360dd4d4afd
CMP Protocol Tests #4167  run 33297180420 / SUCCESS  head 702984933d795dff8a58c11cc15bf0bc68ac9547
CMP Protocol Tests #4168  run 33297201539 / SUCCESS  head 8b2442668a44ae9b115c856120a518a0bb2cb794
CMP Protocol Tests #4169  run 33297342101 / SUCCESS  head a9376388ad80376ca1190f41ef6ff203f4a08584
CMP Protocol Tests #4170  run 33297367289 / SUCCESS  head 914ad8e868a7908eb4386ee1f2bcc180f8ddf3a8
CMP Protocol Tests #4171  run 33297388482 / SUCCESS  head f8e7232009b1aa5e4900e7692256687d0be4704a
```

GitHub metadata подтверждает для #4160–#4171 branch = `arduino-ru-lcd-experiment`, event = `push`, status = `completed`, conclusion = `success` в independently checked chain. Для #4160–#4162 и #4166–#4168 также отдельно проверялся `host-tests`.

Latest exact independently verified GREEN documentation head перед текущими documentation-only updates:

```text
f8e7232009b1aa5e4900e7692256687d0be4704a
CMP Protocol Tests #4171  run 33297388482 / SUCCESS
```

Текущие documentation-only updates после `f8e7232...` нельзя называть GREEN, пока для их exact HEAD не будет собственного SUCCESS. При этом не нужно создавать бесконечную цепочку новых documentation commits только ради записи SUCCESS предыдущего documentation commit, если engineering state не изменился.

Подробный актуальный handoff находится в `15_NEXT_CHAT_TRANSFER_2026-08-30.md`; exact #4160–#4171 snapshot — в `16_CMP_4160_4162_GREEN_2026-08-30.md` (имя файла сохранено для стабильной ссылки, заголовок/содержимое расширены through #4171). Documentation-only runs не заменяют firmware/build evidence checkpoints 166–167.

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