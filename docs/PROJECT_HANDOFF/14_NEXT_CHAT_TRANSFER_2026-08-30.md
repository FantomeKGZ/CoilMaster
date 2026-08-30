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

Текущий подтверждённый pre-update branch HEAD:

```text
8f3d8b4da359b5f5951ab02686f4473ab17086cc
CMP Protocol Tests #4084  run 33291608258 / SUCCESS
```

Последняя подтверждённая documentation-only цепочка:

```text
bbfeaabd2deaa8356300b02f5a6c504907e24922
CMP Protocol Tests #4077  run 33291077232 / SUCCESS

114ec9c3262fa9da61eb6f78cc592306e06aa31f
CMP Protocol Tests #4078  run 33291112419 / SUCCESS

7d340d6b1711420d5e97a6f76acf4920704d098a
CMP Protocol Tests #4079  run 33291149537 / SUCCESS

d62eb851549ad1ce390b34b2c25c54d014a254cc
CMP Protocol Tests #4080  run 33291272960 / SUCCESS

01b209e8a8fd00119f5c1eb54982685737993b38
CMP Protocol Tests #4081  run 33291293949 / SUCCESS

87db3b6d5220da933b94f02bcaf9d917bae26835
CMP Protocol Tests #4082  run 33291400295 / SUCCESS

cb78accce67f73956835137ecb1cd7a2e4701c19
CMP Protocol Tests #4083  run 33291494104 / SUCCESS

8f3d8b4da359b5f5951ab02686f4473ab17086cc
CMP Protocol Tests #4084  run 33291608258 / SUCCESS
```

Для `#4077–#4084`: branch `arduino-ru-lcd-experiment`, status `completed`, conclusion `success`.

`#4080` подтверждает documentation commit `docs(handoff): sync checkpoint 167 latest CI`; `#4081` подтверждает `01b209e8...` (`docs(handoff): record CMP 4077 through 4079`); `#4082` подтверждает `87db3b6d...` (`docs(handoff): record CMP 4080 and 4081`); `#4083` подтверждает `cb78acc...` (`docs(handoff): record exact CMP 4082`); `#4084` подтверждает `8f3d8b4...` (`docs(handoff): advance verified CI through CMP 4083`). Эти runs являются documentation-only confirmations и не заменяют firmware/build evidence checkpoints 166–167. После любого нового docs/code commit снова получать свежий HEAD и не считать его GREEN до отдельного exact run.

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

`WaitingLocalConfirm` остаётся недостижимым LCD state. Hall CGRAM использует только existing glyph bitmap `Д`, `Ч`, `И`, `Л`; после Hall mode normal screen-specific RU glyph set восстанавливается.

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

Former defect where autonomous/completed assignment did not appear in normal motor card remains closed. Static desktop/mobile assignment selectors now contain only canonical `WORKING` / `STARTING`; runtime stale-page filtering and backend unsupported-role rejection remain defense-in-depth.

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

Checkpoint 165 закрыл residual audit как **NO-CHANGE**. Не продолжать speculative refactors только для уменьшения file opens.

Intentional rereads сохраняются:

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

Without a concrete repo defect, the next required engineering gate is physical Arduino + ESP32 E2E on real CoilMaster.

Verify:

1. boot without reset loop;
2. keypad responsiveness;
3. normal RU LCD before Hall mode;
4. Hall armed screen and absence of automatic start;
5. keypad `A` and separate physical START only when interlocks permit;
6. Arduino-only SSR ownership and fail-safe path;
7. readable 15-second Hall countdown;
8. `#` applies/persists accepted calibration; `B` rejects without applying;
9. normal RU glyphs restored after Hall exit;
10. ESP32 loss does not create unsafe start/resume;
11. UART command/ack and Hall telemetry;
12. RUN_STARTED/RUN_COMPLETED evidence behavior;
13. manual exact RUN_WIRE writeoff;
14. reboot/recovery fail-closed behavior.

If hardware E2E exposes a defect, fix only that concrete defect in `arduino-ru-lcd-experiment`, with current-content/current-SHA discipline and exact CI verification.

## 10. Working style

- Russian, concise.
- Execute rather than replace work with long plans.
- Continue code/commits until the concrete repo-reviewable block is closed or a real external blocker exists.
- Do not ask the user to manually verify each commit.
- Never call CI/build GREEN without exact current run confirmation.
