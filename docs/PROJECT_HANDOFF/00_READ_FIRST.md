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

Последние independently verified CI experiment heads перед текущими documentation updates:

```text
b6f40e1cfd6db422a7f506363decc7aa14d2cf9f
CMP Protocol Tests #4127  run 33294728536 / SUCCESS

c834321ec8199d1d3420ed29a625c18760453ec6
CMP Protocol Tests #4126  run 33294630764 / SUCCESS

db0f3175caff1582b95628d85c5edcacf21a59d1
CMP Protocol Tests #4125  run 33294606600 / SUCCESS

de180b5deda6cf1545c439ca09c83da2193c4d30
CMP Protocol Tests #4124  run 33294520980 / SUCCESS

a5059a362844cb8b3668fb38afeea18ba29d552b
CMP Protocol Tests #4123  run 33294500721 / SUCCESS

124e43e7626a769750648cfac96d07c131bd548e
CMP Protocol Tests #4122  run 33294397706 / SUCCESS

5687a7153598879f68f2487e3e61f8241fa60447
CMP Protocol Tests #4121  run 33294372059 / SUCCESS

3c7d82daadffe516ae2c49904ffd750357658526
CMP Protocol Tests #4120  run 33294305854 / SUCCESS

046da69294074ac3ff74990c1d16d8072e8c9380
CMP Protocol Tests #4119  run 33294282887 / SUCCESS
```

GitHub metadata подтверждает для `#4125/#4126/#4127`: branch `arduino-ru-lcd-experiment`, status `completed`, conclusion `success`, event `push`, с exact heads `db0f3175caff1582b95628d85c5edcacf21a59d1`, `c834321ec8199d1d3420ed29a625c18760453ec6` и `b6f40e1cfd6db422a7f506363decc7aa14d2cf9f` соответственно.

Последний exact GREEN documentation head перед новыми handoff updates:

```text
b6f40e1cfd6db422a7f506363decc7aa14d2cf9f
message: docs(handoff): record CMP 4125 and 4126
CMP Protocol Tests #4127  run 33294728536 / SUCCESS
```

Следующий documentation head `f833937bdea98c38a200cce4f297a95d62513d80` (`docs(handoff): advance entrypoint through CMP 4126`) не покрывается `#4127`, потому что `#4127` запущен на его parent `b6f40e1...`. Созданные после него documentation commits также нельзя считать GREEN без собственного exact CI result.

Непосредственно предшествующие documentation heads также подтверждены:

```text
1acb0ba2414c6df04e41f5f00b507d9e54b5924d
CMP Protocol Tests #4118  run 33294172224 / SUCCESS

8614cab2b6b5854187879e1702ded6b1e19210b4
CMP Protocol Tests #4117  run 33294147832 / SUCCESS

699a56937108bbd78963731c891d20d0bb33798e
CMP Protocol Tests #4116  run 33294051310 / SUCCESS

538173ad7b43c8673c4db126391ca705d2a881c4
CMP Protocol Tests #4115  run 33294026938 / SUCCESS

a5da49fd5706843bb129691d6a23288276be12f2
CMP Protocol Tests #4114  run 33293887033 / SUCCESS

e7c5d43184498eb5b003a8c352b0185be9172169
CMP Protocol Tests #4113  run 33293866518 / SUCCESS

ced5d03e66aaa61e1bf2538756e8760622c4e0c8
CMP Protocol Tests #4112  run 33293782544 / SUCCESS

56dfb050993a8ecbf4e1c7ab9692ff7f58555668
CMP Protocol Tests #4111  run 33293760814 / SUCCESS

cbde2c24d9fadf9f7b3a2b048463457183245066
CMP Protocol Tests #4110  run 33293676211 / SUCCESS

9c9b05364d0a00cb801b75159202fc4201e9b0f5
CMP Protocol Tests #4109  run 33293658325 / SUCCESS

cc122bcad7140cb93be7532011180d70de454736
CMP Protocol Tests #4108  run 33293540349 / SUCCESS

8ccf035c04e5791165bd46a6273b31563cb43417
CMP Protocol Tests #4107  run 33293444395 / SUCCESS

578c91d09461bbbe205a0044e70cc2c5fa771195
CMP Protocol Tests #4106  run 33293423944 / SUCCESS

bcc3984bbaeda3e44132573e86c72199c7654521
CMP Protocol Tests #4105  run 33293337241 / SUCCESS
```

Эти documentation-only confirmations не заменяют отдельное firmware/build evidence checkpoints 166–167.

Stable pre-CRM snapshot сохраняется:

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

Static desktop/mobile selectors now expose only canonical `WORKING` / `STARTING`. Runtime stale-page filtering and backend unsupported-role rejection remain defense-in-depth; occupied-role replacement remains explicit and never auto-retried.

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

## Safety invariants

- никакого automatic physical START или repeat START;
- никакого auto-resume after reboot;
- Arduino — единственный владелец SSR;
- ESP32/Web не управляют SSR напрямую;
- `RUN_COMPLETED` остаётся evidence only;
- RUN_WIRE writeoff только explicit/manual;
- exact `spool_id + source_session_id + source_run_id` обязателен;
- mutation-time authoritative rereads / TOCTOU / recovery gates не удалять ради performance;
- confirmed append-only history не редактировать/удалять скрытно;
- no unbounded growing-NDJSON buffering/cache;
- no automatic production truncation/rotation/deletion;
- no premature DB/index migration.