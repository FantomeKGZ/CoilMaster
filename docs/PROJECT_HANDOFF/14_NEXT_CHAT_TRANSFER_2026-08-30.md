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

Последний exact CI-verified handoff HEAD перед текущим documentation update:

```text
026d0e13f535257d85f542795c00e9471483001d
```

Он подтверждён `CMP Protocol Tests #4049`. После него ветка содержит только последующие documentation commits; в новом чате всегда сначала получать свежий branch HEAD и не считать docs-only child новым firmware runtime checkpoint.

## 2. Что читать первым

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

## 3. Current experiment state

Repo-reviewable software work закрыт through checkpoint **166**.

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
```

Checkpoints 159–166 не переделывать без конкретной regression.

## 4. Latest Hall/RU LCD checkpoint — GREEN

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

`WaitingLocalConfirm` остаётся недостижимым LCD state.

Hall CGRAM использует только четыре existing glyph bitmap:

```text
Д, Ч, И, Л
```

После выхода из Hall mode обычный screen-specific RU glyph set обязательно восстанавливается.

Implementation/contracts:

```text
1162bd798b30494b9a04436ea0cd94571e8b6833
15f627c6971520f6dec9ed031e79917cce15cf7e
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c
```

Verified exact source evidence:

```text
CMP Protocol Tests #4028  run 33268897356 / SUCCESS
Arduino RU LCD #206       run 33268897370 / SUCCESS
```

Intermediate `Arduino RU LCD #205` (`33268835043`) был stale source-text contract failure до PlatformIO compile и не является firmware regression.

## 5. Uno memory gate

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

## 6. Repeated-scan optimization status

Checkpoint 165 закрыл residual audit как **NO-CHANGE**.

Не продолжать speculative refactors только для уменьшения file opens.

Intentional rereads, которые сохраняются:

- `SpoolMaterialBridgeIntegrityAudit` cross-journal reference resolution;
- `MaterialUsageCorrectionIntegrityAudit` cumulative correction/provenance checks;
- CashPayment read/preflight vs mutation-time authoritative append reread;
- Repair Intake durable pending/append/recovery rereads;
- любые mutation-time TOCTOU/recovery gates.

Не вводить persistent cache/index/DB, whole-file growing state, unbounded vectors или automatic history truncation/rotation/deletion.

## 7. Autonomous winding canonical projection — closed

Former defect where autonomous/completed assignment did not appear in normal motor card is closed.

Current semantics:

- append-only `MotorWindingVersionStore` projection;
- roles only `WORKING` / `STARTING`;
- exact retry identity `session_id + run_id + role`;
- assignment-only history can backfill on retry;
- target replacement only explicit `replace_existing=true`;
- replacement appends a new version;
- untargeted role preserved completely;
- `STARTING` requires existing `WORKING`;
- UI never auto-retries occupied-role 409;
- no physical RUN evidence fabrication/copying.

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

## 9. Verified handoff CI — 2026-08-30

All listed runs were independently checked against GitHub metadata and completed successfully on `arduino-ru-lcd-experiment`:

```text
bcbc5441f337c53c7b92f956da49f019f4a747a5
docs(handoff): refresh current entrypoint
CMP Protocol Tests #4032  run 33288140386 / SUCCESS

51d1de7839d4f0b7b7be3031546cc896e4bdb212
docs(handoff): add 2026-08-30 transfer checkpoint
CMP Protocol Tests #4033  run 33288156234 / SUCCESS

54ba0370894f4d42617fca36a4fe10611082ec7e
docs(handoff): record verified transfer CI
CMP Protocol Tests #4034  run 33288559791 / SUCCESS

c2d76a1e159733e9972a6e396537710682a84740
docs(handoff): record CMP 4032 and 4033
CMP Protocol Tests #4035  run 33288575129 / SUCCESS

6e109d0c261fcd638c3bdf6922494b298f30d196
docs(handoff): record CMP 4034 and 4035
CMP Protocol Tests #4036  run 33288687699 / SUCCESS

6ff5bbc578a06ef15935085a4048ee487ffaa2f9
docs(handoff): extend verified CI chain
CMP Protocol Tests #4037  run 33288701498 / SUCCESS

89aa0d98d2811b32107b9f3f1ab043517fafe9f6
docs(handoff): record latest verified CMP chain
CMP Protocol Tests #4038  run 33288723882 / SUCCESS

ec28cebb49ea68f2c0222e47b0e9971f8ee40077
docs(handoff): record CMP 4036-4038
CMP Protocol Tests #4039  run 33289007119 / SUCCESS

62274a0fd7e0f40aa2da7768a8f52f46bbb4d891
docs(handoff): extend latest verified CI chain
CMP Protocol Tests #4040  run 33289028681 / SUCCESS

04763307d222c9a9696a4fd396453882744e5a
docs(handoff): record CMP 4039 and 4040
CMP Protocol Tests #4041  run 33289102116 / SUCCESS

fb34936f1e00816000bc5570a0060af0b8ebcca9
docs(handoff): extend verified CI through 4040
CMP Protocol Tests #4042  run 33289117627 / SUCCESS

e72df70cb90a8bcb6bc28d7590c6b91eca09a063
docs(handoff): record CMP 4041
CMP Protocol Tests #4043  run 33289176000 / SUCCESS

944c7c29461f16bb191a2183d913e6db427f0118
docs(handoff): extend verified CI through 4041
CMP Protocol Tests #4044  run 33289192924 / SUCCESS

1bb563db927ad363474f6471252e97c77119ee48
docs(handoff): record CMP 4042 and 4043
CMP Protocol Tests #4045  run 33289269685 / SUCCESS

a50bf196d1e8c88c5a1c578204cb6bebb9e0d5e5
docs(handoff): extend verified CI through 4043
CMP Protocol Tests #4046  run 33289289249 / SUCCESS

1891dd264653dd4724441f4de15a4683c030f528
docs(handoff): extend verified CI through 4046
CMP Protocol Tests #4047  run 33289383083 / SUCCESS

b8f67b5d81611ab5c96110b90adab9461bb4a37b
docs(handoff): record CMP 4044-4046
CMP Protocol Tests #4048  run 33289410136 / SUCCESS

026d0e13f535257d85f542795c00e9471483001d
docs(handoff): sync active CI chain through 4046
CMP Protocol Tests #4049  run 33289445045 / SUCCESS
```

`#4036–#4049` are confirmed SUCCESS from GitHub metadata. `#4036–#4038` additionally had the `host-tests` job explicitly rechecked successful in the prior handoff refresh. `#4046–#4049` were rechecked directly against GitHub metadata in this update. `#4049` is the latest exact CI-verified handoff/documentation head before this commit.

These are documentation/contract handoff checks, not a new firmware runtime checkpoint. The latest exact Hall/RU-LCD firmware evidence remains checkpoint 166 / `#4028` + Arduino RU LCD `#206` above.

## 10. Immediate next work

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

## 11. Working style

- Russian, concise.
- Execute rather than replace work with long plans.
- Continue code/commits until the concrete repo-reviewable block is closed or a real external blocker exists.
- Do not ask the user to manually verify each commit.
- Never call CI/build GREEN without exact current run confirmation.
- Do not copy experiment commits into production without explicit approval.