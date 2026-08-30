# NEXT CHAT TRANSFER — 2026-08-30

Дата: **2026-08-30**  
Репозиторий: **`FantomeKGZ/CoilMaster`**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Активная рабочая ветка: **`arduino-ru-lcd-experiment`**

## 1. Branch policy

- `main` как source не использовать.
- Production `cmp-protocol-v1` не изменять без отдельного прямого запроса пользователя.
- Следующую разработку выполнять только в `arduino-ru-lcd-experiment`.
- Перед изменением существующего файла обязательно получить его актуальное содержимое из текущей ветки и использовать current blob SHA.
- Для нового файла сначала подтвердить, что путь отсутствует.

Production остаётся:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

## 2. Latest independently verified CI handoff

Последняя independently verified documentation chain:

```text
8b3926b8de870589b64f0e0107d2f7a099e89c70
CMP Protocol Tests #4134  run 33295091259 / SUCCESS
message: docs(handoff): advance entrypoint through CMP 4131

b97fd6b9f77496646573bbd1ea64c151c049a78f
CMP Protocol Tests #4133  run 33295068299 / SUCCESS
message: docs(handoff): record CMP 4131

103dc4ef9267c266ae64acadbe2dd198d6a77eed
CMP Protocol Tests #4132  run 33294968670 / SUCCESS
message: docs(handoff): extend verified CI chain through 4130

58dda8de4b76861d05a14390bc3760f4647b7876
CMP Protocol Tests #4131  run 33294943179 / SUCCESS

fd098e0bbd72acc2c7e5c11b397e2315314343d5
CMP Protocol Tests #4130  run 33294860227 / SUCCESS

2390d6e1916c2bd2cbbcb72901cf486e200c72e2
CMP Protocol Tests #4129  run 33294838984 / SUCCESS

f833937bdea98c38a200cce4f297a95d62513d80
CMP Protocol Tests #4128  run 33294756834 / SUCCESS

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

GitHub metadata independently confirms `#4132/#4133/#4134` on branch `arduino-ru-lcd-experiment`, event `push`, status `completed`, conclusion `success`, with exact heads `103dc4ef9267c266ae64acadbe2dd198d6a77eed`, `b97fd6b9f77496646573bbd1ea64c151c049a78f` and `8b3926b8de870589b64f0e0107d2f7a099e89c70` respectively.

Therefore the exact branch HEAD immediately before this documentation update is verified GREEN:

```text
8b3926b8de870589b64f0e0107d2f7a099e89c70
CMP Protocol Tests #4134  run 33295091259 / SUCCESS
```

This new documentation commit must get its own exact CI result before it is itself called GREEN.

The preceding documentation chain through `#4118` is already recorded in repository history and remains valid. These documentation-only confirmations do not replace separate firmware/build evidence for checkpoints 166–167.

## 3. Что читать первым

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

## 4. Current experiment state

Repo-reviewable software work закрыт through checkpoint **167**.

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

Checkpoints 159–167 не переделывать без конкретной regression.

Repeated-scan/performance optimization считается исчерпанной до появления concrete measured bottleneck или дефекта. Не продолжать speculative storage refactors только ради уменьшения file opens.

## 5. Checkpoint 166 — Hall RU LCD firmware

Reachable Hall LCD states in RU build:

```text
ArmedWaitingPhysicalStart
ДАТЧИК ХОЛЛА
A ИЛИ START

Running
ТЕСТ ХОЛЛА
ОСТ. <n> СЕК

WaitingApplyConfirm
СОХР. НАСТР.?
#=ДА B=НЕТ
```

`WaitingLocalConfirm` остаётся недостижимым LCD state. Hall CGRAM использует only existing glyph bitmap `Д`, `Ч`, `И`, `Л`; после Hall mode normal screen-specific RU glyph set восстанавливается.

Exact firmware evidence:

```text
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c
CMP Protocol Tests #4028  run 33268897356 / SUCCESS
Arduino RU LCD #206       run 33268897370 / SUCCESS
```

Exact #206 build sizes:

```text
uno_ru_lcd
RAM   1614 / 2048 = 78.8%
Flash 31448 / 32256 = 97.5%
Flash headroom = 808 bytes

uno fallback
RAM   1605 / 2048 = 78.4%
Flash 31066 / 32256 = 96.3%
Flash headroom = 1190 bytes
```

Следствие: broad Uno feature growth остановить. Новые Arduino-side изменения допустимы только при конкретном дефекте и должны быть минимальными. Расширенную обработку/представление по возможности переносить на ESP32, не нарушая независимую безопасную работу Arduino.

## 6. Checkpoint 167 — canonical winding-role selector

Former defect where autonomous/completed assignment did not appear in normal motor card remains closed. Static desktop/mobile assignment selectors contain only canonical `WORKING` / `STARTING`; runtime stale-page filtering and backend unsupported-role rejection remain defense-in-depth.

Occupied-role replacement remains explicit `replace_existing=true`, append-only and never auto-retried.

Exact code/build evidence:

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

## 7. Repeated-scan optimization status

Checkpoint 165 закрыл residual audit как **NO-CHANGE**. Intentional rereads сохраняются:

- `SpoolMaterialBridgeIntegrityAudit` cross-journal reference resolution;
- `MaterialUsageCorrectionIntegrityAudit` cumulative correction/provenance checks;
- CashPayment read/preflight vs mutation-time authoritative append reread;
- Repair Intake durable pending/append/recovery rereads;
- любые mutation-time TOCTOU/recovery gates.

Не вводить persistent cache/index/DB, whole-file growing state, unbounded vectors или automatic history truncation/rotation/deletion.

## 8. Safety invariants — do not change

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

## 9. Immediate next work

Без конкретного repo defect следующий обязательный engineering gate — physical Arduino + ESP32 E2E на реальном CoilMaster.
