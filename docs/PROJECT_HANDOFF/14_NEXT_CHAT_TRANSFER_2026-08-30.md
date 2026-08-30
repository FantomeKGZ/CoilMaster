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

Свежая exact verified documentation chain:

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
```

GitHub metadata independently confirms #4140 and #4141 on branch `arduino-ru-lcd-experiment`, event `push`, status `completed`, conclusion `success`.

Exact latest verified GREEN documentation head before the current documentation updates:

```text
24300c90a77f17c8b4592b2f41932ea46ae34279
CMP Protocol Tests #4141  run 33295478482 / SUCCESS
```

The documentation then advanced first to:

```text
70ca869607d3556d3e37fc413c6ee557fdd442fd
message: docs(handoff): record CMP 4140 and 4141
```

and then to the commit containing this transfer-file update. Those newer documentation commits require their own exact CI results before they are called GREEN.

Earlier documentation verification remains valid in Git history. Documentation-only confirmations do not replace separate firmware/build evidence for checkpoints 166–167.

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

Во время E2E проверить минимум:

1. ESP32 command → Arduino ack.
2. Keypad responsiveness before/after Hall mode.
3. Normal RU LCD screens before Hall, Hall screens during test, normal CGRAM restoration after exit.
4. Physical START ownership only on Arduino; no Web/ESP32 SSR path.
5. Hall 15-second run, apply and reject paths.
6. SSR fail-safe behavior.
7. RUN_STARTED/RUN_COMPLETED evidence without automatic wire deduction.
8. Manual exact RUN_WIRE writeoff with `spool_id + source_session_id + source_run_id`.
9. Reboot/recovery fail-closed behavior and no auto-resume.
