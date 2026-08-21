# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints сохраняются как история и **не являются backlog**.

## Last verified production baseline

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

Пользователь после documentation cleanup сообщил, что workflows зелёные, но current production candidate ниже содержит новые ESP32 safety/integrity changes и требует своих exact automated results перед присвоением GREEN.

## Current production candidate — checkpoint 62

Production hardening:

```text
ce38711c  Keep timed-out jobs in manual review
2ecebb5e  Ignore stale cancel after completed job
9e68bd86  Reject runs beyond immutable repeat target
5fa6bcea  Validate session state against repeat target
```

Regression guards:

```text
fa5a0073  Guard timed-out job manual review
44d1e037  Guard stale cancel after completed job
5188084d  Guard immutable repeat target journal boundary
5e407f2e  Guard repeat target session integrity
```

Current safety semantics:

- lost ACK / `TIMED_OUT` never proves Arduino idle;
- ordinary dismiss cannot close `TIMED_OUT`;
- positive cancel closes only zero-run waiting state;
- stale cancel after `ProgramCompleted`/`ClosedAfterReview` is a no-op on persisted run evidence;
- final repeat target is checked before journal append;
- `RUN_STARTED` cannot reopen an already completed target;
- `RUN_COMPLETED` above immutable target is rejected before NDJSON mutation;
- deep session audit validates `JobRuntimeState.completedRuns` against immutable snapshot `repeatTarget` without another full journal scan.

Current candidate verification required:

```text
ESP32 Build — NOT VERIFIED
CMP Protocol Tests — NOT VERIFIED
```

Arduino production code/wire format этим candidate не менялись.

## Закрытые блоки — не поднимать без регрессии

Не считать следующей работой:

- старую реализацию JOB cancel/recovery (`ALREADY_CLEAR`, `ALL_CLEAR`, physical fallback), кроме проверки checkpoint 62;
- repeat-target UI/model implementation как новую функцию — сейчас проверяется только новый final-boundary hardening;
- Arduino autonomous archive UI/provenance;
- motor schema/detail/repair-history contracts;
- KG_FIRST quantity/writeoff/costing compatibility;
- writeoff fault-path hardening;
- NDJSON observability/rotation strategy groundwork;
- warehouse/winding bounded scan hardening;
- backup whitelist/deep integrity/session preflight consolidation;
- web regression-contract recovery;
- obsolete duplicate ESP32 job-lifecycle implementation.

Если старый checkpoint говорит `next`, но этот раздел помечает блок закрытым, считать старый `next` историческим.

## Текущая активная очередь

### 1. Verify checkpoint 62 candidate

Получить exact results:

```text
ESP32 Build
CMP Protocol Tests
```

Если красный — исправлять только фактическую compile/test ошибку.

### 2. Targeted two-board UART/hardware smoke

После green automated gates, при доступном стенде:

```text
normal JOB delivery
-> Arduino READY
-> physical START only
-> RUN_STARTED
-> RUN_COMPLETED
-> event ACK/replay
-> final repeat remains terminal
-> no automatic material writeoff
```

Cancel/recovery smoke:

```text
no-run JOB -> cancel
already-clear cancel
physical D -> * -> # -> D fallback
lost JOB_ACK / timeout -> manual review, not ordinary dismiss/new-job bypass
late ALL_CLEAR after completed job -> no corruption/storage fault
```

### 3. Дальнейшая работа

После current gates выбирать задачу только из:

- конкретного bug report;
- измеренного performance/storage hotspot;
- явно запрошенной новой функциональности;
- подтверждённого persistence/integrity fault.

Не начинать speculative database migration, destructive compaction или новую rotation policy без измерений.

## Safety boundary

Не менять:

- physical START только physical/local;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web do not drive SSR directly;
- timeout/lost ACK never proves Arduino idle;
- final repeat cannot reopen automatically;
- RUN_COMPLETED never auto-writes off material;
- manual writeoff requires exact source session/run provenance;
- KG_FIRST may omit spool only in approved unallocated/manual path;
- exact spool provenance retained when a spool is used;
- backup/restore remains explicit, operator-only and fail-closed.

## Verification language

```text
GREEN/SUCCESS — named workflow/test actually completed successfully
FAILED        — named gate actually ran and failed
NOT VERIFIED  — result is unavailable/not run
USER CONFIRMED — user explicitly verified real hardware behavior
```

Git commit сам по себе не является build result.

## Документация для продолжения

Перед работой читать:

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/62_JOB_LIFECYCLE_SAFETY_HARDENING_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Исторические checkpoints читать только для деталей уже выполненной реализации или evidence конкретного старого baseline.
