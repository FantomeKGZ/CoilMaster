# NEXT CHAT TRANSFER — 2026-08-30

Дата: **2026-08-30**  
Репозиторий: **`FantomeKGZ/CoilMaster`**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Активная рабочая ветка: **`arduino-ru-lcd-experiment`**

## 1. Branch policy

- `main` как source **не использовать**.
- Production `cmp-protocol-v1` не изменять без отдельного прямого запроса пользователя.
- Вся дальнейшая разработка выполняется только в `arduino-ru-lcd-experiment`.
- Перед изменением существующего файла обязательно получить его актуальное содержимое именно из `arduino-ru-lcd-experiment` и использовать текущий blob SHA.
- Для нового файла сначала проверить, что путь отсутствует.

Production остаётся неизменённым:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

HEAD, подтверждённый перед этим handoff update:

```text
1238bd68b2c6b947148c0a7f47e10c0a42eb20fb
```

Этот HEAD — documentation-only commit `docs(handoff): sync latest verified CI chain`.

## 2. Что читать в новом чате

Сначала прочитать:

- `/AGENTS.md`
- `docs/PROJECT_HANDOFF/00_READ_FIRST.md`
- `docs/PROJECT_HANDOFF/01_CURRENT_STATE.md`
- `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`
- `docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md`
- `docs/PROJECT_HANDOFF/10_CHECKPOINT_161_WAREHOUSE_PROVENANCE_SUFFIX_SCAN.md`
- `docs/PROJECT_HANDOFF/11_CHECKPOINT_162_REPAIR_FINALIZATION_KNOWN_REPAIR.md`
- `docs/PROJECT_HANDOFF/12_CHECKPOINT_163_165_REPEATED_SCAN_CLOSEOUT.md`
- `docs/PROJECT_HANDOFF/08_NEXT_CHAT_TRANSFER_2026-08-29.md`
- `docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md`

## 3. Текущее состояние experiment

Repo-reviewable experiment work закрыт through checkpoint **167**.

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
166 reachable Hall RU LCD localization
167 static canonical winding-role selector cleanup
```

Checkpoints **159–167** считать закрытыми, пока не появится конкретная regression или измеренный bottleneck.

## 4. Checkpoint 166 — Hall RU LCD — GREEN

Reachable Hall screens now use the Russian LCD path without changing Hall calibration control flow.

Exact final source evidence:

```text
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c
CMP Protocol Tests #4028  run 33268897356 / SUCCESS
Arduino RU LCD #206       run 33268897370 / SUCCESS
```

Uno build sizes from #206:

```text
uno_ru_lcd: RAM 1614 / 2048 (78.8%); Flash 31448 / 32256 (97.5%); headroom 808 bytes
uno:        RAM 1605 / 2048 (78.4%); Flash 31066 / 32256 (96.3%); headroom 1190 bytes
```

Because RU build headroom is only **808 bytes**, avoid broad Uno-side feature growth. Prefer ESP32 for processing/expanded presentation where architecture permits.

## 5. Checkpoint 167 — canonical winding-role selector — GREEN

Desktop and mobile autonomous winding assignment pages now statically expose only canonical roles:

- `WORKING`
- `STARTING`

`AUXILIARY` was removed from static HTML. Runtime filtering remains defense-in-depth. Backend still rejects unsupported roles and occupied-role replacement remains explicit-only.

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

## 6. Latest verified documentation/CI chain

The later runs after checkpoint 167 are documentation-only confirmations and do not replace the exact firmware/build evidence above.

Latest independently verified run:

```text
HEAD 1238bd68b2c6b947148c0a7f47e10c0a42eb20fb
CMP Protocol Tests #4085
run 33291646267
completed / SUCCESS
branch arduino-ru-lcd-experiment
```

GitHub metadata confirms run #4085 belongs exactly to HEAD `1238bd68...`, branch `arduino-ru-lcd-experiment`, and completed successfully.

Previously recorded documentation-only confirmations include `#4066–#4067` and `#4071–#4076`; they do not substitute for the exact checkpoint 166/167 runtime evidence.

## 7. Residual repeated-scan audit — checkpoint 165 NO-CHANGE

Do not continue speculative storage refactoring merely to reduce file opens.

Reviewed and intentionally retained:

- `SpoolMaterialBridgeIntegrityAudit` arbitrary cross-reference scans;
- `MaterialUsageCorrectionIntegrityAudit` batch rereads required for cumulative correction/provenance proof;
- CashPayment read/preflight vs mutation append reread separation;
- Repair Intake/recovery rereads around durable pending/append.

No persistent cache/index/DB, whole-file buffering or unbounded RAM should be introduced without a concrete measured need and proof-preserving design.

## 8. Autonomous winding canonical projection — checkpoint 159 CLOSED

The former defect where completed autonomous winding assignment did not appear in the normal motor card is closed.

Current semantics:

- assignment projects into append-only `MotorWindingVersionStore`;
- canonical roles only `WORKING` and `STARTING`;
- exact retry identity `session_id + run_id + role`;
- historical assignment-only records backfill on retry;
- occupied target role never silently overwrites;
- replacement requires explicit `replace_existing=true` and appends a new canonical version;
- untargeted role is preserved completely;
- `STARTING` without existing `WORKING` fails closed;
- UI never automatically retries an occupied-role conflict;
- no physical RUN evidence is fabricated or rewritten.

## 9. Safety / integrity invariants — не менять

- никакого automatic physical START или repeat START;
- никакого auto-resume after reboot;
- Arduino остаётся единственным владельцем SSR;
- ESP32/Web никогда не управляют SSR напрямую;
- `RUN_COMPLETED` остаётся только evidence и сам по себе не списывает провод;
- RUN_WIRE writeoff остаётся явным/manual;
- exact `spool_id + source_session_id + source_run_id` обязателен;
- restore/recovery fail closed/operator controlled;
- MaterialLedger mutation-time TOCTOU/authoritative reread сохраняются;
- разные integrity domains не объединять только ради уменьшения I/O;
- никаких unbounded in-RAM scans/cache растущих NDJSON;
- никакой автоматической truncation/rotation/deletion production history;
- никакой преждевременной миграции в DB/index;
- никакой silent edit/delete append-only history.

## 10. Что делать дальше

1. В новом рабочем цикле сначала заново получить current HEAD `arduino-ru-lcd-experiment`.
2. Не переделывать checkpoints 159–167 без конкретной regression.
3. Repeated-scan optimization считать исчерпанным до измеренного bottleneck/конкретного дефекта.
4. Продолжать только конкретные experiment/Hall/RU-LCD defects или другую явно подтверждённую функциональную задачу.
5. С учётом 808 bytes RU flash headroom избегать широкого роста Uno-кода.
6. По возможности переносить processing/expanded UI logic на ESP32, сохраняя независимую безопасную работу Arduino.
7. Перед каждым изменением существующего файла fetch current content + blob SHA.
8. После runtime изменения проверять фактические CMP + relevant ESP32/Arduino builds.
9. Не утверждать GREEN без exact current run result.
10. Production `cmp-protocol-v1` не трогать без отдельного запроса пользователя.

## 11. Hardware acceptance

Full two-board Arduino + ESP32 E2E остаётся обязательным финальным acceptance gate перед release completion.

Финальная hardware проверка должна подтвердить:

- UART command/ack flow;
- physical START ownership on Arduino;
- Hall calibration/telemetry behavior;
- keypad/LCD behavior, включая RU-LCD experiment;
- RUN_STARTED/RUN_COMPLETED evidence;
- manual exact RUN_WIRE writeoff;
- reboot/recovery fail-closed behavior.

## 12. Стиль работы

- Писать по-русски и кратко.
- Не заменять реализацию длинным планом.
- Продолжать кодом/commit-ами без остановки, пока repo-reviewable блок не закрыт или не возникнет реальный внешний blocker.
- Не просить пользователя вручную проверять каждый commit.
- Не сообщать, что build/CI GREEN, пока это не подтверждено.