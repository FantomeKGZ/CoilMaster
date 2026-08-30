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

## Current work stream — feature completeness

По прямому запросу пользователя начат полный аудит всех ранее обсуждавшихся добавлений/обновлений функций. Цель: проверить реальную реализацию, закончить proven incomplete items и только после этого переходить к следующему этапу.

Это не новый speculative storage/performance audit. Checkpoints 159–167 остаются закрытыми, если нет concrete regression.

Первый найденный functional gap уже исправлен: Web calculator раньше принимал несколько исходных диаметров, но всегда отправлял `source_strands_N=1`. Desktop/mobile теперь поддерживают explicit strand count per component, например `0,80x3`, `1,00x5`, `0,80x3;1,00x2`, используя уже существующий backend range 1..12.

Commits:

```text
4c6554a07b5e4ff8104ef0b9d8fc0914677ff9d5  desktop calculator
 da6b5423d782b73ed4ebacb9aaf5fa164d5ac552  mobile parity
1b7f8504184b681d5f7e0da7710c4a50601a346a  regression contract
```

## Latest exact CI

Recent verified runs:

```text
CMP #4519 run 33312797865 / SUCCESS head 626399bc84ac31cc791fc298376a7da666b2a85a
CMP #4520 run 33312873604 / SUCCESS head b4510d4dca30b6d33be1d8c0d03087515b5e75a5
CMP #4521 run 33312892924 / SUCCESS head 395d474732902650397c9ec53cb308d6e3c93b74
CMP #4522 run 33312914746 / SUCCESS head 268383ac8d8ae12c03e952f2919f8a9c162d3e65
CMP #4523 run 33313015445 / SUCCESS head 7eee9bc312bbaf07308e8213c9738cf8cda3202b
CMP #4524 run 33313040747 / SUCCESS head 32da60826ee39e410afef0c30b69fc1e3fd63158
CMP #4525 run 33313307355 / FAILURE head 4c6554a07b5e4ff8104ef0b9d8fc0914677ff9d5
CMP #4526 run 33313331225 / FAILURE head da6b5423d782b73ed4ebacb9aaf5fa164d5ac552
CMP #4527 run 33313347671 / SUCCESS head 1b7f8504184b681d5f7e0da7710c4a50601a346a
ESP32 #1780 run 33313307362 / SUCCESS head 4c6554a07b5e4ff8104ef0b9d8fc0914677ff9d5
```

#4525/#4526 are understood intermediate failures: the old `Audit calculator source wire input` contract still required `source_strands_N=1`. The functional implementation itself reached ESP32 build success on desktop at #1780. The test contract was then aligned and CMP recovered to SUCCESS at #4527.

Latest exact CMP SUCCESS before this documentation update:

```text
1b7f8504184b681d5f7e0da7710c4a50601a346a
CMP #4527 run 33313347671 / SUCCESS
```

Do not infer ESP32/build GREEN for later SHA without exact run evidence.

## Existing closed engineering state

Experiment-side previous software checkpoints remain closed through **167**:

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

Physical Arduino+ESP32 E2E for the previously tested hardware/firmware state remains operator-confirmed PASS.

## Feature-completeness audit order

Continue systematically with:

1. clients/motors/repairs pagination;
2. motor import workflow;
3. shared Web shell/navigation/global search/recent/breadcrumbs/time/version/toasts;
4. FTP/Web recovery;
5. Wi-Fi profiles/static IP/`coil.local`;
6. backup/settings;
7. desktop/mobile feature parity;
8. stale/empty pages and links;
9. any other previously documented promised feature found incomplete.

For every proven gap: fetch exact current file + blob SHA, implement minimal fix, add/update regression coverage, verify applicable CI, then update HANDOFF meaningfully.

## Firmware/build evidence retained

Checkpoint 166 Hall RU LCD:

```text
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c
CMP #4028 run 33268897356 / SUCCESS
Arduino RU LCD #206 run 33268897370 / SUCCESS
```

Uno resource evidence:

```text
uno_ru_lcd: RAM 1614 / 2048 (78.8%); Flash 31448 / 32256 (97.5%); headroom 808 bytes
uno:        RAM 1605 / 2048 (78.4%); Flash 31066 / 32256 (96.3%); headroom 1190 bytes
```

Checkpoint 167 remains verified by CMP/ESP32/Arduino runs recorded in the detailed handoff.

## Safety invariants

Do not change:

- no automatic physical START/repeat START;
- no auto-resume after reboot;
- Arduino is sole SSR owner;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically writes off wire;
- manual linked RUN_WIRE requires exact `spool_id + source_session_id + source_run_id`;
- restore/recovery remain fail-closed/operator-controlled;
- mutation-time authoritative rereads/TOCTOU guards remain;
- append-only evidence is not silently edited/deleted;
- no unbounded growing-NDJSON cache;
- no automatic production truncation/rotation/deletion;
- no premature DB/index migration.

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

Continue the feature-completeness audit from fresh branch HEAD. Do not return to documentation-only CI recursion as the main activity. Production `cmp-protocol-v1` remains untouched.