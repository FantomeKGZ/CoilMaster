# CoilMaster — current project entrypoint

Дата обновления: **2026-09-03**  
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

## Current feature block — new motor captures canonical WORKING / STARTING winding

Блок создания/редактирования/просмотра обмотки двигателя закрыт через checkpoint 18.

Ключевой контракт сохраняется:

```text
GET  /api/motors/winding/latest?motor_id=...
POST /api/motors/winding/role
optimistic token = expected_winding_version_id
stale token      = HTTP 409 winding_version_conflict
storage          = append-only motor-winding-versions.ndjson
```

При создании новой карточки:

- `WORKING` обязателен;
- `STARTING` создаётся только при включённом `Есть пусковая обмотка`;
- материал задаётся независимо для каждой роли: `CU` / Медь или `AL` / Алюминий;
- для каждой роли принимается 1–5 физических проводов;
- пользовательские разделители диаметров: `;` или `:`, можно смешивать;
- десятичный знак: запятая или точка;
- одинаковые диаметры разрешены и canonical-нормализуются через `xN`;
- legacy `coil_program` / `repeat_target` берутся из WORKING;
- первая versioned запись всегда WORKING; STARTING использует возвращённый version id и не может стать orphan first version.

Canonical conductor model остаётся единым: `CU/AL:<hundredths_mm>xN`, без второго wire-storage формата. `MotorWindingRoleSpec::MaxConductors = 5U`.

Desktop/mobile `motor-edit.html` показывают friendly `Материал + Провод`, но сохраняют raw canonical fallback для legacy/смешанных записей. `motor-details.html` показывает материал и диаметры человекочитаемо и сохраняет raw fallback для неизвестного формата.

Detailed record:
```text
docs/PROJECT_HANDOFF/18_CHECKPOINT_MOTOR_NEW_WINDING_CAPTURE_2026-09-03.md
```

Exact implementation GREEN before documentation refresh:
```text
bb6af586d51ace86a584ec3f24e5a96b7d4e9e0d
CMP Protocol Tests #4770 run 33708384422 / SUCCESS
```

Более новые docs commits нельзя называть GREEN без собственного exact SUCCESS.

## Previous feature block — motor WORKING / STARTING edit

В `arduino-ru-lcd-experiment` добавлено редактирование канонических ролей обмотки непосредственно в desktop/mobile `motor-edit.html`.

Редактирование одной роли сохраняет вторую роль из latest без изменений. Первая versioned запись создаётся через `WORKING`; `STARTING` без существующей `WORKING` fail-closed. При `409` UI перечитывает latest и не выполняет автоматический повтор mutation.

Detailed record:
```text
docs/PROJECT_HANDOFF/17_CHECKPOINT_WORKING_STARTING_EDIT_2026-08-31.md
```

Intermediate exact CI evidence:
```text
43d405c40b5cc042a4bc78e92340a8546d031c73
CMP Protocol Tests #4651 run 33355313297 / SUCCESS
```

## First closed feature gap — calculator source strand counts

Web calculator поддерживает explicit strand count per component:
```text
0,80x3
1,00x5
0,80x3;1,00x2
```
Без `xN` сохраняется один strand. Backend уже валидирует 1..12 strands/component.

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

## Historical exact documentation CI

```text
CMP #4572 run 33315540094 / SUCCESS head 5bf08ea0439ad8f67624b6db17c692da7f8dc333
CMP #4573 run 33315616257 / SUCCESS head f00c7d64151c0604b72296156c8ee4531321364e
CMP #4574 run 33315633625 / SUCCESS head e4af32aa5361fbafada845d8cabcd259f4e106dd
CMP #4575 run 33315653163 / SUCCESS head 3486dda09f29775b46cc60dc4231ba984ab7c669
CMP #4576 run 33315752219 / SUCCESS head 76b1e6d60404aaf715b9fa23b5e55b50205c9e6a
CMP #4577 run 33315776327 / SUCCESS head 5b6b3eebd4e133e893227a48eefdf8036cba2555
CMP #4578 run 33315795903 / SUCCESS head df4b1251b1d38b90ed0f5ef6ce26b92d794fd0bd
```

#4575 verifies transfer through #4572. #4576/#4577/#4578 independently verify snapshot/entrypoint/transfer HANDOFF through #4574. Thus all HANDOFF documentation through `df4b1251b1d38b90ed0f5ef6ce26b92d794fd0bd` is independently CMP-GREEN.

Documentation-only CI recursion must not become the main activity.

## Feature-completeness audit order

Continue systematically with:
1. clients/motors/repairs bounded pagination;
2. shared Web shell/navigation/global search/recent/breadcrumbs/time/version/toasts;
3. FTP/Web recovery;
4. Wi-Fi profiles/static IP/`coil.local`;
5. backup/settings;
6. desktop/mobile feature parity;
7. stale/empty pages and links;
8. any other previously promised feature found incomplete.

Motor import and new-motor canonical WORKING/STARTING capture are closed unless a concrete regression is found.

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
docs/PROJECT_HANDOFF/18_CHECKPOINT_MOTOR_NEW_WINDING_CAPTURE_2026-09-03.md
docs/PROJECT_HANDOFF/17_CHECKPOINT_WORKING_STARTING_EDIT_2026-08-31.md
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

First verify exact CI for this documentation HEAD. After that continue the feature-completeness audit from bounded clients/motors/repairs pagination unless the user selects another concrete feature. Production `cmp-protocol-v1` remains untouched.
