# NEXT CHAT TRANSFER — 2026-08-30 — checkpoint after CMP #4159

Дата: **2026-08-30**  
Репозиторий: **`FantomeKGZ/CoilMaster`**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Активная рабочая ветка: **`arduino-ru-lcd-experiment`**

## Branch policy

- `main` как source не использовать.
- Production `cmp-protocol-v1` не изменять без отдельного прямого запроса пользователя.
- Все дальнейшие experiment-side изменения выполнять только в `arduino-ru-lcd-experiment`.
- Перед изменением существующего файла обязательно fetch current branch content и использовать current blob SHA.
- Для нового файла сначала подтверждать отсутствие пути.

Production остаётся неизменённым:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

## Exact current handoff state

GitHub metadata независимо подтверждает свежую documentation chain:

```text
CMP Protocol Tests #4154
run 33296258713
head a3082500e9295ee38823456ff69c8b6530b369da
branch arduino-ru-lcd-experiment
event push
status completed
conclusion SUCCESS
message docs(handoff): checkpoint exact CMP 4153 state

CMP Protocol Tests #4155
run 33296340509
head f99bdcf3f108243e3192c8e83a66b289469681ed
branch arduino-ru-lcd-experiment
event push
status completed
conclusion SUCCESS
message docs(handoff): confirm CMP 4154 after 4153 checkpoint

CMP Protocol Tests #4156
run 33296357807
head 00919cbf5e8cd847a0e622bdbfe7bf4b291ab7f5
branch arduino-ru-lcd-experiment
event push
status completed
conclusion SUCCESS
message docs(handoff): advance entrypoint through CMP 4154

CMP Protocol Tests #4157
run 33296440587
head dc9ff401cbb2e5dc68b0311c6a28f142462d7cab
branch arduino-ru-lcd-experiment
event push
status completed
conclusion SUCCESS
message docs(handoff): record CMP 4155 and 4156

CMP Protocol Tests #4158
run 33296459453
head 945061512a705a5b4f61a054841c977b7e978c9e
branch arduino-ru-lcd-experiment
event push
status completed
conclusion SUCCESS
message docs(handoff): advance through CMP 4156

CMP Protocol Tests #4159
run 33296578402
head bd7e1f8454a25305fc0a4af47361341ba161d84f
branch arduino-ru-lcd-experiment
event push
status completed
conclusion SUCCESS
message docs(handoff): record CMP 4157 and 4158
```

Следовательно `bd7e1f8454a25305fc0a4af47361341ba161d84f` — latest exact independently verified GREEN documentation head по известному CI:

```text
CMP Protocol Tests #4159  run 33296578402 / SUCCESS
```

После него уже существовал documentation-only commit:

```text
d8a597c1ac07ee234a824a419698a4dc61067761
message: docs(handoff): advance through CMP 4158
```

А обновление `00_READ_FIRST.md`, записывающее #4159, создало:

```text
124464fc728414c6ba770669755a57e724e4710c
message: docs(handoff): record CMP 4159
```

`d8a597c1...`, `124464fc...` и commit этого файла нельзя называть GREEN, пока GitHub Actions не даст exact SUCCESS для соответствующего HEAD.

## Current engineering state

Repo-reviewable experiment-side software work закрыт through checkpoint **167**:

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

Checkpoints 159–167 считать закрытыми и не переделывать без конкретной regression.

Repeated-scan/performance optimization считается исчерпанной до появления concrete measured bottleneck или дефекта. Не продолжать speculative storage refactors только ради уменьшения file opens.

## Latest firmware/build evidence that remains authoritative

Checkpoint 166 Hall RU LCD:

```text
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c
CMP Protocol Tests #4028  run 33268897356 / SUCCESS
Arduino RU LCD #206       run 33268897370 / SUCCESS
```

Uno sizes from #206:

```text
uno_ru_lcd: RAM 1614 / 2048 (78.8%); Flash 31448 / 32256 (97.5%); headroom 808 bytes
uno:        RAM 1605 / 2048 (78.4%); Flash 31066 / 32256 (96.3%); headroom 1190 bytes
```

Checkpoint 167 canonical winding-role selector:

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

Documentation-only CI after these checkpoints does not replace their firmware/build evidence.

## Immediate next work

Без нового concrete repo defect следующий обязательный engineering gate — **physical Arduino + ESP32 E2E** на реальном CoilMaster.

Минимальный acceptance checklist:

1. ESP32 command -> Arduino ack.
2. Keypad responsiveness до и после Hall mode.
3. Normal RU LCD screens до Hall, Hall screens during test, normal CGRAM restoration после выхода.
4. Physical START ownership только на Arduino; Web/ESP32 не управляют SSR.
5. Hall 15-second run, apply и reject paths.
6. SSR fail-safe behavior.
7. RUN_STARTED/RUN_COMPLETED evidence без automatic wire deduction.
8. Manual exact RUN_WIRE writeoff с `spool_id + source_session_id + source_run_id`.
9. Reboot/recovery fail-closed behavior; no auto-resume.

Если до hardware E2E обнаружен конкретный software defect, исправлять его минимально в `arduino-ru-lcd-experiment`, сохраняя существующие safety/integrity границы.

## Safety invariants — do not change

- no automatic physical START/repeat START;
- no auto-resume after reboot;
- Arduino is sole SSR owner;
- ESP32/Web never controls SSR directly;
- `RUN_COMPLETED` is evidence only;
- RUN_WIRE writeoff remains explicit/manual;
- exact `spool_id + source_session_id + source_run_id` mandatory;
- restore/recovery remain fail-closed/operator-controlled;
- mutation-time authoritative rereads and TOCTOU guards remain;
- confirmed append-only history is never silently edited/deleted;
- no unbounded growing-NDJSON buffering/cache;
- no automatic production truncation/rotation/deletion;
- no premature DB/index migration.

## Read order for continuation

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
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```