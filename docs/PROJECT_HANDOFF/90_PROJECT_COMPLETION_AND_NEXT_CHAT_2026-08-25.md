# CoilMaster — completion estimate and next-chat transfer

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Repo: `FantomeKGZ/CoilMaster`

Этот checkpoint является текущим authoritative transfer для продолжения проекта в новом чате. Старые numbered checkpoints остаются history/evidence и не являются активным backlog.

## Completion estimate

```text
Core software architecture / production flow      ~99%
ESP32 services, Web, persistence, backup          ~98%
Arduino Uno runtime / CMP1 / Hall split            ~97%
Integrity, recovery, CI regression coverage        ~99%
Reference site / SD web bundle                     ~98%
Full two-board hardware acceptance                 pending final E2E
Overall release readiness                          ~95%
```

Software repo-only Stage-1 optimization is now **closed**. The remaining mandatory release gate is one complete hardware acceptance on real ESP32 + Arduino Uno. NDJSON threshold/rotation decisions remain deferred until real device metrics exist.

## Source of truth / working rules

- Единственная source-of-truth ветка: **`cmp-protocol-v1`**.
- `main` не использовать как источник кода.
- Перед изменением existing file: fetch exact current content из `cmp-protocol-v1` + current blob SHA.
- Перед созданием new file: проверить exact path и убедиться в 404/not found.
- Не объявлять CI/build/hardware GREEN без фактической проверки или явного подтверждения оператора.
- После meaningful change обновлять `docs/PROJECT_HANDOFF`.

## Safety invariants — never weaken

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly drives SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts wire/material;
- writeoff stays explicit/manual and tied to exact `source_session_id + source_run_id + immutable spool_id`;
- cancellation never erases immutable run/history evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion or NDJSON truncation.

## Production ownership

```text
ESP32: service/data/UI orchestration, SD/RTC/network, workshop registry,
       jobs/persistence, warehouse/material/costing, backup/restore,
       Web UI and extended Hall calibration analysis/history

CMP1 UART: commands/jobs/config down; run/status/calibration events up

Arduino Uno: physical START, SSR authority, normal Hall realtime count,
             keypad/LCD/buzzer, local calibration safety gates,
             realtime winding state machine and RUN events
```

Production wire protocol remains text `CMP1|...`. `Shared/Protocol/` is older host/test protocol code and is not the production replacement.

## Production flow

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> exact immutable spool selection + immutable snapshot
-> UART JOB -> physical START
-> RUN_STARTED / RUN_COMPLETED
-> explicit manual exact-run exact-spool writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

## Hall architecture

Uno owns realtime Hall threshold/hysteresis/debounce/direction and local physical safety. ESP32 owns raw sample aggregation, baseline/min/max/span/count/duration, recommendation, history, Web/status and proposal orchestration.

```text
ESP32 CAL_ARM
-> Uno WAITING_LOCAL_CONFIRM
-> local #
-> ARMED_WAITING_START
-> baseline
-> separate physical START
-> RUNNING
-> CAL_SAMPLE stream + CAL_DONE measurement_id
-> ESP32 analysis/recommendation
-> CAL_PROPOSAL exact measurement_id
-> Uno WAITING_APPLY_CONFIRM
-> local #
-> EEPROM apply
```

ESP32 intentionally keeps legacy `CAL_RESULT` receive fallback. Uno no longer emits it. Lost `CAL_APPLIED` does not replay proposal; CFG_GET may reconcile settings but equality does not prove the exact apply event.

## Uno resource state

Latest verified production size baseline:

```text
RAM   1205 / 2048 = 58.8%   free 843 B
Flash 31460 / 32256 = 97.5%  free 796 B
```

Flash is limiting. CI guard requires at least 512 B free RAM and at least 512 B free flash.

## Latest fully GREEN implementation block

**92_MATERIAL_USAGE_SINGLE_PASS_PREFLIGHT**

```text
72401aae0d1b34fbb211ce92c48d0a367f337b91  perf implementation
8ce55052f98d491f3f1f2fda4830955e87159798  regression guard
6d77ac1b4ad7fcc25cc1873d5e0c13e819011ece  CMP workflow wiring
```

Block 92 replaces two equivalent preflight reads of `/data/materials/materials.ndjson` inside `MaterialLedger::confirmUsage()` with one authoritative `readMaterialState()` pass. Exact material identity, ACTIVE state, stock, price and currency remain fail-closed. `rewriteQuantity()` remains the separate transactional mutation/revalidation boundary.

Verified successful tail on `cmp-protocol-v1`:

```text
CMP #3111 / run 32831517018 / SUCCESS
ESP32 Build #1443 / run 32831517073 / SUCCESS
CMP #3112 / run 32831547926 / SUCCESS
CMP #3113 / run 32831593193 / SUCCESS
CMP #3114 / run 32831715701 / SUCCESS
CMP #3115 / run 32831755533 / SUCCESS
CMP #3116 / run 32831820366 / SUCCESS
CMP #3117 / run 32831861941 / SUCCESS
```

The Block-92 implementation/test/CI chain is the firmware/software GREEN baseline. Later docs-only commits do not constitute a newer firmware implementation baseline.

## Stage-1 software optimization closure

Authoritative closure checkpoint:

```text
docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md
```

No further repo-only optimization is justified without new evidence. Final reviewed KEEP candidates include:

- `MaterialLedgerWeb::handleUsage()` HTTP prevalidation vs core mutation validation;
- warehouse write-off Web validation vs core/transaction revalidation;
- warehouse price Web no-op detection vs service-level integrity/no-op protection;
- repair-status bounded self-scan;
- autonomous assignment event batching;
- warehouse movement provenance uniqueness batching;
- legacy ESP32 `CAL_RESULT` receive fallback;
- Uno resource-sensitive buffers/contracts.

Do not continue optimization merely to reduce scan count when different passes enforce different API, integrity, recovery, or transaction semantics.

## NDJSON strategy

No premature database migration. No automatic cleanup/rotation. `/api/system/storage` provides growth observability. Threshold/rotation policy must be based on measured real-device data after hardware acceptance.

## Next mandatory gate — final hardware acceptance

Perform one complete ESP32 + Arduino Uno acceptance:

```text
1. boot both boards / CMP1 handshake/status
2. deliver one linked JOB; Uno receives it but does not auto-start
3. physical START only
4. RUN_STARTED observed on ESP32
5. stable normal Hall turn counting
6. RUN_COMPLETED observed on ESP32
7. no automatic wire/material writeoff
8. manual exact-run exact-spool writeoff
9. repeat requires another physical START; final repeat cannot auto-reopen
10. cancel/recovery
11. reboot: no auto-resume / no physical auto-start
12. Hall calibration full ARM -> local confirm -> physical START -> CAL_SAMPLE/CAL_DONE -> proposal -> local apply -> CFG reconciliation
13. keypad/LCD/buzzer usable
14. SSR controlled only by Uno
```

This is the final external hardware gate, not an intermediate test.

## Read order for a new chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md
docs/PROJECT_HANDOFF/92_MATERIAL_USAGE_SINGLE_PASS_PREFLIGHT_2026-08-25.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

## Ready-to-paste prompt for the next chat

```text
Продолжаем проект CoilMaster.

Репозиторий: FantomeKGZ/CoilMaster.
Единственная source-of-truth ветка: cmp-protocol-v1. main для исходников не использовать.

Сначала прочитай /AGENTS.md, docs/PROJECT_HANDOFF/00_READ_FIRST.md, docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md и docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md.

Stage-1 software optimization закрыт. Последний полностью GREEN implementation block: 92_MATERIAL_USAGE_SINGLE_PASS_PREFLIGHT.
Implementation 72401aae0d1b34fbb211ce92c48d0a367f337b91; regression 8ce55052f98d491f3f1f2fda4830955e87159798; CI 6d77ac1b4ad7fcc25cc1873d5e0c13e819011ece. ESP32 Build #1443 run 32831517073 SUCCESS; CMP #3113 run 32831593193 SUCCESS; последующие CMP #3114-#3117 также SUCCESS.

Не продолжай repo-only optimization без новых measured evidence. Следующий обязательный шаг — один полный двухплатный hardware acceptance ESP32 + Arduino Uno по checkpoint 93. Safety-инварианты не менять: никакого automatic physical START, auto-resume, ESP32 SSR control или automatic writeoff; writeoff только manual exact source_session_id + source_run_id + immutable spool_id.
```
