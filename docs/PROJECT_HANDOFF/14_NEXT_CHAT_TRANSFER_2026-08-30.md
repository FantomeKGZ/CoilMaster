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
e1fa715c8049e765b0c5bc010a95a5fafadac614
```

Он подтверждён `CMP Protocol Tests #4076` (`33290785679`) / SUCCESS. Последовательная documentation-only chain также подтверждена: `#4073` (`33290636544`) на `578e4f786ab67bb701b1c796bd3e137492e46e6e`, `#4074` (`33290727723`) на `37ffd3d41ca7602c9e8ff60086c124d9692f3ff8`, `#4075` (`33290759157`) на `76feab5c37973a5ad4afc63ef515d5b6f7f4fdc3` и `#4076` на `e1fa715c8049e765b0c5bc010a95a5fafadac614`. Эти documentation-only runs не заменяют exact code/build evidence checkpoint 167. После текущего documentation update в новом чате всегда сначала получать свежий branch HEAD и не считать его GREEN без отдельного exact run.

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

## 7. Autonomous winding canonical projection / role selector — closed

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
- desktop/mobile static selectors now also contain only `WORKING` / `STARTING`;
- runtime stale-page filtering remains defense-in-depth;
- no physical RUN evidence fabrication/copying.

Checkpoint 167 exact code/build evidence:

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
bcbc5441f337c53c7b92f956da49f019f4a747a5  CMP #4032  run 33288140386 / SUCCESS
51d1de7839d4f0b7b7be3031546cc896e4bdb212  CMP #4033  run 33288156234 / SUCCESS
54ba0370894f4d42617fca36a4fe10611082ec7e  CMP #4034  run 33288559791 / SUCCESS
c2d76a1e159733e9972a6e396537710682a84740  CMP #4035  run 33288575129 / SUCCESS
6e109d0c261fcd638c3bdf6922494b298f30d196  CMP #4036  run 33288687699 / SUCCESS
6ff5bbc578a06ef15935085a4048ee487ffaa2f9  CMP #4037  run 33288701498 / SUCCESS
89aa0d98d2811b32107b9f3f1ab043517fafe9f6  CMP #4038  run 33288723882 / SUCCESS
ec28cebb49ea68f2c0222e47b0e9971f8ee40077  CMP #4039  run 33289007119 / SUCCESS
62274a0fd7e0f40aa2da7768a8f52f46bbb4d891  CMP #4040  run 33289028681 / SUCCESS
04763307d222c9a9696a4fd396453882744e5a  CMP #4041  run 33289102116 / SUCCESS
fb34936f1e00816000bc5570a0060af0b8ebcca9  CMP #4042  run 33289117627 / SUCCESS
e72df70cb90a8bcb6bc28d7590c6b91eca09a063  CMP #4043  run 33289176000 / SUCCESS
944c7c29461f16bb191a2183d913e6db427f0118  CMP #4044  run 33289192924 / SUCCESS
1bb563db927ad363474f6471252e97c77119ee48  CMP #4045  run 33289269685 / SUCCESS
a50bf196d1e8c88c5a1c578204cb6bebb9e0d5e5  CMP #4046  run 33289289249 / SUCCESS
1891dd264653dd4724441f4de15a4683c030f528  CMP #4047  run 33289383083 / SUCCESS
b8f67b5d81611ab5c96110b90adab9461bb4a37b  CMP #4048  run 33289410136 / SUCCESS
026d0e13f535257d85f542795c00e9471483001d  CMP #4049  run 33289445045 / SUCCESS
fd3eb34a15ccdc334202f38d165934ae0bb1f2ce  CMP #4050  run 33289542271 / SUCCESS
248e6a1861ef310aba38910043124f4777833943  CMP #4051  run 33289561938 / SUCCESS
8d64b08eb05959eb0aa1112c936176e3bab36969  CMP #4052  run 33289583224 / SUCCESS
60da9ea34c0fb2998aded8ff6dd35b28c0273b29  CMP #4053  run 33289676773 / SUCCESS
a4ac7b02bb68dec2f8cc1887f9206188fb7b105e  CMP #4054  run 33289697240 / SUCCESS
78901d2f730e431411582c2e46e796560908b6c6  CMP #4055  run 33289767490 / SUCCESS
8177d383c8b0aa5b94658d3bb59fd86ff066a62e  CMP #4056  run 33289786730 / SUCCESS
1579b6a2c202457e501dc7a8aa3480ae0ce0702e  CMP #4057  run 33289866054 / SUCCESS
46cede22a06fdf7dc68dfce545c618ced9063876  CMP #4058  run 33289891342 / SUCCESS
70e4db3aad53458bef5778b7c8cd5c7a08ec2840  CMP #4059  run 33289916765 / SUCCESS
f04ee9e8f36b29cde590c0a364a6a95442bc56b3  CMP #4060  run 33289985842 / SUCCESS
08d79f3a06ea473fff0644efe4540c5251942c39  CMP #4061  run 33290011908 / SUCCESS
f90ae58c163880491a40d9c23409984801835acc  CMP #4062  run 33290126788 / SUCCESS
b189d7d3575663fa2f11b376352dca1cda301377  CMP #4063  run 33290149645 / SUCCESS
05b5fd3a2acb2f8cb5d7f167848561bc353864fe  CMP #4064  run 33290236422 / SUCCESS
8fd0a99e5240bdd30bf590d7ffe9e1ccf361712d  CMP #4065  run 33290257905 / SUCCESS
8baf7119dd2f962032ed655ac39a1ffbb85abe6b  CMP #4066  run 33290353792 / SUCCESS
52c2ed9015df7591fc730a9614b4e8c85d2bb3bb  CMP #4067  run 33290379205 / SUCCESS
9e538828ed179700d362286a3af72de6a6ce0b6f  CMP #4068  run 33290408963 / SUCCESS
47903b0f2e2ddc8ac90abf1e26db7e678a570363  CMP #4069  run 33290422893 / SUCCESS
0eb32376de3a4c50c765dcbe6b946524d075f69b  CMP #4070  run 33290440543 / SUCCESS
5ed169dc7ef0ac16768810d44bda732da2233b4f  CMP #4071  run 33290487906 / SUCCESS
fcbcc2b1dba77f2962e9e733d6f78ed931aa6c52  CMP #4072  run 33290608524 / SUCCESS
578e4f786ab67bb701b1c796bd3e137492e46e6e  CMP #4073  run 33290636544 / SUCCESS
37ffd3d41ca7602c9e8ff60086c124d9692f3ff8  CMP #4074  run 33290727723 / SUCCESS
76feab5c37973a5ad4afc63ef515d5b6f7f4fdc3  CMP #4075  run 33290759157 / SUCCESS
e1fa715c8049e765b0c5bc010a95a5fafadac614  CMP #4076  run 33290785679 / SUCCESS
```

`#4066–#4067` and `#4071–#4076` are documentation-only handoff confirmations. `#4068/#1778/#207` and `#4069/#1779/#208` are exact checkpoint-167 code/build evidence; `#4070` is exact source-text regression-contract evidence.

These documentation runs do not replace exact Hall/RU-LCD firmware evidence checkpoint 166 (`CMP #4028` + Arduino RU LCD `#206`) and do not make later documentation HEADs firmware checkpoints.

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
