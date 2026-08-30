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

Текущий exact CI-verified experiment HEAD перед этим documentation update:

```text
7d340d6b1711420d5e97a6f76acf4920704d098a
CMP Protocol Tests #4079  run 33291149537 / SUCCESS
```

Непосредственно предшествующая подтверждённая documentation-only chain:

```text
bbfeaabd2deaa8356300b02f5a6c504907e24922
CMP Protocol Tests #4077  run 33291077232 / SUCCESS

114ec9c3262fa9da61eb6f78cc592306e06aa31f
CMP Protocol Tests #4078  run 33291112419 / SUCCESS

7d340d6b1711420d5e97a6f76acf4920704d098a
CMP Protocol Tests #4079  run 33291149537 / SUCCESS
```

`#4077–#4079` проверены по GitHub metadata: branch `arduino-ru-lcd-experiment`, status `completed`, conclusion `success`. Это documentation-only подтверждения и они не заменяют exact firmware/build evidence checkpoints 166–167 ниже.

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