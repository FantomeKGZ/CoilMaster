# NEXT CHAT TRANSFER — 2026-08-29

Дата: **2026-08-29**  
Репозиторий: **`FantomeKGZ/CoilMaster`**  
Production/source-of-truth: **`cmp-protocol-v1`**  
Активная рабочая ветка: **`arduino-ru-lcd-experiment`**

## 1. Branch policy

- `main` как source **не использовать**.
- Production `cmp-protocol-v1` не изменять без отдельного прямого запроса пользователя.
- Вся следующая разработка выполняется только в `arduino-ru-lcd-experiment`.
- Перед изменением существующего файла обязательно получить его актуальное содержимое именно из `arduino-ru-lcd-experiment` и использовать текущий blob SHA.
- Для нового файла сначала проверить, что путь отсутствует.

Production остаётся:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

Последний exact runtime HEAD, подтверждённый перед handoff update:

```text
fb7aaa368ae21fe5041395f0df5eef959233920d
```

После документационных коммитов фактический HEAD ветки будет новее; в новом чате всегда сначала заново получить current branch HEAD.

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

Repo-reviewable software work подтверждён through checkpoint **165**.

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
```

Checkpoints 159–165 считаются закрытыми, если не обнаружена конкретная regression.

## 4. Последний runtime checkpoint — 164 GREEN

`SpoolMaterialBridgeStore::validateAll()` теперь не перечитывает уже доказанный prefix bridge journal для каждого fixed batch. Current batch проверяется внутри bounded RAM и только против ещё не доказанного suffix, начинающегося с `outer.position()`.

Коммиты блока:

```text
d8862aef7ae3b3c4a6e3e7dbbe49c92d19babb77  suffix scan implementation
63c70e59fee99f77c20135606c8d9911f8bfbd4e  scoped contract
8f37cb5268ee461e9b41a2307981d3fd45b9a565  stale contract correction
fb7aaa368ae21fe5041395f0df5eef959233920d  final namespace-close fix
```

Exact final CI:

```text
CMP Protocol Tests #4014  run 33266181118 / SUCCESS
ESP32 Build #1777         run 33266181104 / SUCCESS
```

Intermediate failures are documented and must not be confused with GREEN:

```text
CMP #4011   run 33266038272 / FAILURE  stale source-text expectation
ESP32 #1776 run 33266038221 / FAILURE  missing namespace CM closing brace
```

## 5. Checkpoint 163 — Repair Delivery GREEN

Repair Delivery append fuses existing-repair linkage/conflict validation and next `delivery_id` allocation into one authoritative delivery-journal pass.

Verified final evidence:

```text
ESP32 Build #1774         run 33265057626 / SUCCESS
Arduino RU LCD #198       run 33265057605 / SUCCESS
CMP Protocol Tests #4009  run 33265221419 / SUCCESS
CMP Protocol Tests #4010  run 33265276200 / SUCCESS
```

Earlier `#4005–#4008`, `#1773`, `#197` are intermediate failures only.

## 6. Checkpoint 159 functional defect — CLOSED

The former defect where autonomous/completed winding assignment did not appear in the normal motor card is closed.

Current canonical semantics:

- assignment projects into append-only `MotorWindingVersionStore`;
- roles only `WORKING` and `STARTING`;
- operator explicitly selects role;
- `AUXILIARY` canonical projection is rejected;
- exact retry identity is `session_id + run_id + role`;
- historical assignment-only records backfill on retry;
- target role replacement requires explicit `replace_existing=true`;
- replacement appends a new full canonical version and does not rewrite history;
- untargeted role is preserved completely;
- `STARTING` without existing `WORKING` fails closed;
- UI does not auto-retry occupied-role conflict;
- no physical RUN evidence is fabricated/copied.

## 7. Residual repeated-scan audit — checkpoint 165 NO-CHANGE

Do not continue speculative storage refactoring merely to reduce file opens.

Reviewed and intentionally retained:

- `SpoolMaterialBridgeIntegrityAudit` cross-reference batch scans: referenced spool/material IDs may live anywhere in their journals; suffix-only lookup is not equivalent without a new index/cache.
- `MaterialUsageCorrectionIntegrityAudit` batch rereads: required for cumulative over-correction, operation uniqueness and source provenance.
- CashPayment read/preflight vs mutation append reread: separate integrity phases.
- Repair Intake/recovery rereads around durable pending/append: intentional TOCTOU/recovery boundaries.

No persistent cache/index/DB, whole-file buffering or unbounded RAM was introduced.

Detailed reasoning: `12_CHECKPOINT_163_165_REPEATED_SCAN_CLOSEOUT.md`.

## 8. Safety / integrity invariants — не менять

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

## 9. Что делать дальше

Repeated-scan/performance audit временно исчерпан. Следующий кодовый блок должен появляться только из конкретного дефекта, измеренного bottleneck либо оставшейся Hall/RU-LCD experiment acceptance задачи.

Если продолжается repo-only работа без hardware:

1. Получить current HEAD `arduino-ru-lcd-experiment`.
2. Проверить `01_CURRENT_STATE.md` и `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
3. Не переделывать закрытые checkpoints 159–165 без конкретной regression.
4. Для Hall/RU-LCD брать только конкретный ещё не закрытый contract/defect; существующие Hall safety contracts уже входят в CMP suite.
5. Перед каждым изменением существующего файла fetch current content + blob SHA.
6. После runtime изменения проверять фактические CMP + relevant ESP32/Arduino builds.
7. Не утверждать GREEN без exact current run result.
8. Production `cmp-protocol-v1` не трогать.

## 10. Hardware acceptance

Full two-board Arduino + ESP32 E2E остаётся обязательным финальным acceptance gate перед release completion.

Промежуточные hardware tests для уже закрытых repo-reviewable software checkpoints не требуются, но финальная проверка двух плат должна подтвердить:

- UART command/ack flow;
- physical START ownership on Arduino;
- Hall calibration/telemetry behavior;
- keypad/LCD behavior, включая RU-LCD experiment;
- RUN_STARTED/RUN_COMPLETED evidence;
- manual exact RUN_WIRE writeoff;
- reboot/recovery fail-closed behavior.

## 11. Стиль работы

- Писать по-русски и кратко.
- Не заменять реализацию длинным планом.
- Продолжать кодом/commit-ами без остановки, пока repo-reviewable блок не закрыт или не возникнет реальный внешний blocker.
- Не просить пользователя вручную проверять каждый commit.
- Не сообщать, что build/CI GREEN, пока это не подтверждено.