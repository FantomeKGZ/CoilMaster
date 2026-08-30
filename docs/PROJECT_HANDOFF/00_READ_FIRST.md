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

GitHub metadata подтверждает свежую documentation chain на `arduino-ru-lcd-experiment`:

```text
13c26dad7b01f10545d1019cb130d350f26cef89
CMP Protocol Tests #4137  run 33295262217 / SUCCESS

9b62ddd9ce6cfff1a9075635dad5f3b81c785ef0
CMP Protocol Tests #4138  run 33295354492 / SUCCESS

ae15755f99d9c545a057e50cc483e56da1173838
CMP Protocol Tests #4139  run 33295375686 / SUCCESS

f7ee334f5a2fac874e335729e3cca7d0ab1b8351
CMP Protocol Tests #4140  run 33295459355 / SUCCESS
message: docs(handoff): advance verified head through CMP 4139

24300c90a77f17c8b4592b2f41932ea46ae34279
CMP Protocol Tests #4141  run 33295478482 / SUCCESS
message: docs(handoff): record CMP 4137 through 4139

70ca869607d3556d3e37fc413c6ee557fdd442fd
CMP Protocol Tests #4142  run 33295556344 / SUCCESS
message: docs(handoff): record CMP 4140 and 4141

0fd178e2d4a194b707420edfa1c4f525cad2fbdf
CMP Protocol Tests #4143  run 33295571380 / SUCCESS
message: docs(handoff): extend verified CI through CMP 4141

902d128bca63dbde58831834b17b8b6b3b1cdfa6
CMP Protocol Tests #4144  run 33295649674 / SUCCESS
message: docs(handoff): record CMP 4142 and 4143

cfcff39961bc7ba5d4f48e726f60f31aa79ec62d
CMP Protocol Tests #4145  run 33295750743 / SUCCESS
message: docs(handoff): record CMP 4144

a5854e74d66b0333de488f4836e8cf6160d3eadc
CMP Protocol Tests #4146  run 33295769234 / SUCCESS
message: docs(handoff): advance entrypoint through CMP 4144
```

Для #4140–#4146 GitHub metadata независимо подтверждает branch = `arduino-ru-lcd-experiment`, event = `push`, status = `completed`, conclusion = `success`.

Следовательно latest exact independently verified GREEN head перед текущим documentation-only обновлением:

```text
a5854e74d66b0333de488f4836e8cf6160d3eadc
CMP Protocol Tests #4146  run 33295769234 / SUCCESS
```

Этот текущий documentation-only commit должен получить собственный exact CI result прежде чем новый documentation HEAD можно называть GREEN.

Более длинная verified documentation chain остаётся в `14_NEXT_CHAT_TRANSFER_2026-08-30.md` и Git history. Documentation-only runs не заменяют firmware/build evidence checkpoints 166–167.

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

## Safety invariants

- no automatic physical START/repeat START;
- no auto-resume after reboot;
- Arduino is sole SSR owner;
- ESP32/Web never controls SSR directly;
- `RUN_COMPLETED` is evidence only;
- RUN_WIRE writeoff remains explicit/manual;
- exact `spool_id + source_session_id + source_run_id` mandatory;
- restore/recovery fail closed/operator controlled;
- mutation-time authoritative rereads and TOCTOU guards remain;
- append-only confirmed history never silently edited/deleted;
- no unbounded growing-NDJSON buffering/cache;
- no automatic production truncation/rotation/deletion;
- no premature DB/index migration.