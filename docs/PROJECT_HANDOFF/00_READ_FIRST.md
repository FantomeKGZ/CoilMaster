# CoilMaster — current project entrypoint

Дата обновления: **2026-08-25**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Что читать новому AI / coding agent

```text
/AGENTS.md
this file
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

`90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md` — authoritative transfer checkpoint.

`93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md` — current phase transition: Stage-1 repo-only optimization is closed and the next mandatory gate is one full two-board hardware acceptance.

`92_MATERIAL_USAGE_SINGLE_PASS_PREFLIGHT_2026-08-25.md` — latest fully GREEN implementation block.

Старые numbered checkpoints — history/evidence, а не backlog. Не продолжать старую задачу только потому, что исторический checkpoint содержит `next`/`pending`.

Перед изменением/deletion existing file обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить exact path. Не утверждать CI/build/hardware GREEN без фактического результата или явного подтверждения оператора. Empty GitHub code-search не является достаточным доказательством отсутствия dependency.

## Current completion estimate

```text
Overall project readiness                         ~95%
Software/repo implementation + integrity          ~98-99%
Reference Web/site layer                          ~98%
Full two-board hardware acceptance                still required
```

Основной production flow, persistence, Web, backup, Hall split, Uno runtime, safety contracts и Stage-1 storage/performance hardening собраны. Repo-only optimization phase закрыта.

## Current verified GREEN software baseline

Latest verified implementation block: **92_MATERIAL_USAGE_SINGLE_PASS_PREFLIGHT**.

```text
72401aae0d1b34fbb211ce92c48d0a367f337b91
perf(esp32): collapse material usage preflight scan

8ce55052f98d491f3f1f2fda4830955e87159798
regression guard

6d77ac1b4ad7fcc25cc1873d5e0c13e819011ece
CMP workflow wiring

ESP32 Build #1443 / run 32831517073 / SUCCESS
CMP #3111 / run 32831517018 / SUCCESS
CMP #3112 / run 32831547926 / SUCCESS
CMP #3113 / run 32831593193 / SUCCESS
CMP #3114 / run 32831715701 / SUCCESS
CMP #3115 / run 32831755533 / SUCCESS
CMP #3116 / run 32831820366 / SUCCESS
CMP #3117 / run 32831861941 / SUCCESS
```

Block 92 collapses two equivalent material-catalog preflight reads in `MaterialLedger::confirmUsage()` into one authoritative `readMaterialState()` pass while retaining `rewriteQuantity()` as the separate transactional mutation/revalidation pass.

Documentation-only commits after the Block-92 implementation chain do not establish a newer firmware implementation baseline. Hardware GREEN cannot be inferred from CI.

## Current phase — final external hardware gate

Do **not** continue repo-only optimization without new measured evidence. The remaining repeated reads reviewed after Block 92 enforce different HTTP, integrity or transaction semantics and are KEEP.

Next mandatory step: one complete hardware acceptance on real ESP32 + Arduino Uno:

```text
1. boot + CMP1 handshake
2. linked JOB delivery without auto-start
3. physical START only
4. RUN_STARTED
5. stable Hall count
6. RUN_COMPLETED
7. no automatic writeoff
8. manual exact source_session_id + source_run_id + immutable spool_id writeoff
9. repeat requires another physical START; final repeat cannot auto-reopen
10. cancel/recovery
11. reboot -> no auto-resume / no automatic physical start
12. Hall calibration full ARM/local confirm/physical start/CAL_SAMPLE/CAL_DONE/proposal/local apply/CFG reconciliation
13. keypad/LCD/buzzer
14. SSR authority Uno-only
```

This is the final hardware acceptance, not an intermediate test.

## Safety invariants

Never weaken:

- physical START only physical/local;
- no automatic physical START between repeat cycles;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK / timeout never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never performs automatic wire/material writeoff;
- linked-production manual writeoff requires exact `source_session_id + source_run_id + immutable spool_id`;
- operational cancellation does not erase immutable run/history evidence;
- backup restore operator-only, transactional and fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion/truncation.

## NDJSON rule

No premature DB migration. No automatic cleanup/rotation. Thresholds only from measured real-device data after hardware acceptance.

Точная current state, closure decision, KEEP list and hardware acceptance sequence находятся в:

```text
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md
```
