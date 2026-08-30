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

Все новые изменения выполнять только в `arduino-ru-lcd-experiment`. Не переносить experiment обратно в production без отдельного прямого запроса пользователя. Перед изменением существующего файла fetch exact current content + blob SHA; для нового файла сначала подтверждать отсутствие пути.

## Current work stream — feature completeness

По прямому запросу пользователя идёт полный аудит ранее обсуждавшихся обновлений/добавлений функций. Цель: проверить реальную реализацию, закончить proven incomplete items и только после этого переходить к следующему этапу.

Previous checkpoints 159–167 остаются закрытыми, если нет concrete regression. Speculative repeated-scan/performance work не переоткрывать.

## First closed feature gap — calculator source strand counts

Web calculator раньше принимал несколько исходных диаметров, но всегда отправлял `source_strands_N=1`. Теперь desktop/mobile поддерживают explicit strand count per component:

```text
0,80x3
1,00x5
0,80x3;1,00x2
```

Без `xN` сохраняется один strand. Backend уже валидирует 1..12 strands/component; backend safety semantics не расширялись.

Commits:

```text
4c6554a07b5e4ff8104ef0b9d8fc0914677ff9d5  desktop calculator
da6b5423d782b73ed4ebacb9aaf5fa164d5ac552  mobile parity
1b7f8504184b681d5f7e0da7710c4a50601a346a  regression contract
```

Exact build/CI evidence:

```text
ESP32 #1780 run 33313307362 / SUCCESS head 4c6554a07b5e4ff8104ef0b9d8fc0914677ff9d5
Arduino RU LCD #209 run 33313307363 / SUCCESS head 4c6554a07b5e4ff8104ef0b9d8fc0914677ff9d5
ESP32 #1781 run 33313331248 / SUCCESS head da6b5423d782b73ed4ebacb9aaf5fa164d5ac552
Arduino RU LCD #210 run 33313331284 / SUCCESS head da6b5423d782b73ed4ebacb9aaf5fa164d5ac552
CMP #4527 run 33313347671 / SUCCESS head 1b7f8504184b681d5f7e0da7710c4a50601a346a
```

CMP #4525/#4526 were understood intermediate failures only in `Audit calculator source wire input`; the old contract still required one strand per entered diameter. It was updated in `1b7f8504...`, after which CMP recovered at #4527.

## Latest exact documentation CI

```text
CMP #4548 run 33314417096 / SUCCESS head e056dc9eefc597847905c0f850b9db1e4a8b11e3
CMP #4549 run 33314549761 / SUCCESS head 5f62b3e5017f5ed57c53760aefad0c279fcf6631
CMP #4550 run 33314575279 / SUCCESS head 579b9423db0f31d1d8d0d289056de6ed1423a97c
CMP #4551 run 33314598022 / SUCCESS head 8709a6f118d0c100c2fa9d6620128c60e1d8b7a5
CMP #4552 run 33314693290 / SUCCESS head bf3270f1b3875a5938da7565dc6d0fbf2616b000
CMP #4553 run 33314714837 / SUCCESS head fdc56cbefa6fe05ccb258234ff9edbd6553caf32
```

#4552 verifies snapshot through #4551. #4553 verifies entrypoint through #4551. Transfer through #4551 (`a39c17c0f672ef14cce3a11a65e1cac6607d146d`) remains without its own supplied exact CMP SUCCESS and must not be called GREEN.

Latest exact independently verified GREEN SHA before this documentation refresh:

```text
fdc56cbefa6fe05ccb258234ff9edbd6553caf32
CMP #4553 run 33314714837 / SUCCESS
```

New docs commits after #4553 require their own exact run before being called GREEN. Documentation-only CI recursion must not become the main activity.

## Feature-completeness audit order

Continue systematically with:

1. clients/motors/repairs bounded pagination;
2. motor import workflow;
3. shared Web shell/navigation/global search/recent/breadcrumbs/time/version/toasts;
4. FTP/Web recovery;
5. Wi-Fi profiles/static IP/`coil.local`;
6. backup/settings;
7. desktop/mobile feature parity;
8. stale/empty pages and links;
9. any other previously promised feature found incomplete.

For every proven gap: fetch exact current file + blob SHA, implement minimal fix, add/update regression coverage, verify applicable CI, then update HANDOFF meaningfully.

## Physical / prior engineering state

Physical Arduino+ESP32 E2E for the previously tested hardware/firmware state remains operator-confirmed PASS. Checkpoints 159–167 remain closed unless concrete regression is found.

Uno checkpoint 166 resource evidence remains:

```text
uno_ru_lcd: RAM 1614 / 2048 (78.8%); Flash 31448 / 32256 (97.5%); headroom 808 bytes
uno:        RAM 1605 / 2048 (78.4%); Flash 31066 / 32256 (96.3%); headroom 1190 bytes
```

## Safety invariants

Do not change: no automatic physical START/repeat START; no auto-resume after reboot; Arduino is sole SSR owner; ESP32/Web never directly controls SSR; `RUN_COMPLETED` never automatically writes off wire; manual linked RUN_WIRE requires exact `spool_id + source_session_id + source_run_id`; restore/recovery remain fail-closed/operator-controlled; mutation-time authoritative rereads/TOCTOU guards remain; append-only evidence is not silently edited/deleted; no automatic production truncation/rotation/deletion; no premature DB/index migration.

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
docs/PROJECT_HANDOFF/15_NEXT_CHAT_TRANSFER_2026-08-30.md
docs/PROJECT_HANDOFF/16_CMP_4160_4162_GREEN_2026-08-30.md
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```

## Immediate NEXT

Continue the feature-completeness audit from fresh branch HEAD, beginning with clients/motors/repairs pagination unless the user selects another concrete feature. Do not return to documentation-only CI recursion as the main activity. Production `cmp-protocol-v1` remains untouched.