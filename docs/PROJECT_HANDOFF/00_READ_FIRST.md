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

Последний exact CI-verified handoff HEAD:

```text
04763307d222c9a9696a6a4fd396453882744e5a
```

Текущий pre-update branch HEAD был docs-only child:

```text
fb34936f1e00816000bc5570a0060af0b8ebcca9
```

Verified handoff CI chain:

```text
bcbc5441f337c53c7b92f956da49f019f4a747a5
CMP Protocol Tests #4032  run 33288140386 / SUCCESS

51d1de7839d4f0b7b7be3031546cc896e4bdb212
CMP Protocol Tests #4033  run 33288156234 / SUCCESS

54ba0370894f4d42617fca36a4fe10611082ec7e
CMP Protocol Tests #4034  run 33288559791 / SUCCESS

c2d76a1e159733e9972a6e396537710682a84740
CMP Protocol Tests #4035  run 33288575129 / SUCCESS

6e109d0c261fcd638c3bdf6922494b298f30d196
CMP Protocol Tests #4036  run 33288687699 / SUCCESS

6ff5bbc578a06ef15935085a4048ee487ffaa2f9
CMP Protocol Tests #4037  run 33288701498 / SUCCESS

89aa0d98d2811b32107b9f3f1ab043517fafe9f6
CMP Protocol Tests #4038  run 33288723882 / SUCCESS

ec28cebb49ea68f2c0222e47b0e9971f8ee40077
CMP Protocol Tests #4039  run 33289007119 / SUCCESS

62274a0fd7e0f40aa2da7768a8f52f46bbb4d891
CMP Protocol Tests #4040  run 33289028681 / SUCCESS

04763307d222c9a9696a6a4fd396453882744e5a
CMP Protocol Tests #4041  run 33289102116 / SUCCESS
```

`#4032–#4041` — documentation/contract handoff checks. `#4041` independently подтверждён GitHub metadata: `completed/success`, branch `arduino-ru-lcd-experiment`, exact head `04763307d222c9a9696a6a4fd396453882744e5a`. Они не заменяют firmware/runtime evidence checkpoint 166; последний точный Arduino RU LCD firmware checkpoint остаётся `#4028` + Arduino RU LCD `#206` ниже.

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

Experiment-side repo-reviewable software work закрыт through checkpoint **166**.

Закрытые последние блоки:

```text
159 autonomous winding -> canonical motor history projection
160 Warehouse exact lookup optimization
161 Warehouse CONFIRMED provenance suffix scan
162 repair finalization known-repair proof reuse
163 Repair Delivery single-pass append preflight
164 spool/material bridge suffix uniqueness audit
165 residual repeated-scan audit -> NO-CHANGE
166 reachable Hall RU LCD localization -> GREEN
```

Repeated-scan/performance optimization считается исчерпанной до появления конкретного измеренного bottleneck или дефекта. Не продолжать speculative storage refactors только ради уменьшения количества file opens.

## Latest verified Hall/RU LCD source checkpoint

```text
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c
CMP Protocol Tests #4028  run 33268897356 / SUCCESS
Arduino RU LCD #206       run 33268897370 / SUCCESS
```

Uno resource evidence из exact #206:

```text
uno_ru_lcd: RAM 1614 / 2048 (78.8%); Flash 31448 / 32256 (97.5%); headroom 808 bytes
uno:        RAM 1605 / 2048 (78.4%); Flash 31066 / 32256 (96.3%); headroom 1190 bytes
```

Практический вывод: не расширять Uno крупными Hall/UI-функциями. Новые Arduino-side изменения только для конкретных дефектов и максимально малы; расширенную обработку/представление по возможности держать на ESP32, сохраняя автономную безопасность Arduino.

Подробно: `13_HALL_RU_LCD_ACCEPTANCE.md`.

## Immediate NEXT

1. Получить свежий HEAD `arduino-ru-lcd-experiment` перед любым изменением.
2. Не переделывать checkpoints 159–166 без конкретной regression.
3. Следующая repo-only работа должна идти только от конкретного дефекта, измеренного bottleneck либо hardware-acceptance находки.
4. Full Arduino + ESP32 hardware E2E остаётся обязательным финальным acceptance gate.
5. Во время hardware E2E проверить UART command/ack, keypad, normal RU screens before/after Hall, physical START ownership, Hall 15-second run/apply/reject, SSR fail-safe, RUN evidence, manual exact RUN_WIRE writeoff и reboot/recovery fail-closed behavior.
6. Перед каждым изменением существующего файла fetch current branch content + current blob SHA.
7. Не утверждать GREEN без exact current CI/run evidence.
8. Production `cmp-protocol-v1` не трогать без отдельного запроса.

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