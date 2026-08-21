# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints сохраняются как история и **не являются backlog**.

## Current verified repo baseline — checkpoint 62

ESP32 production C++ hardening завершён на:

```text
5fa6bcea812c33f0b2dc8e13baae476221839b3a
Validate session state against repeat target
```

Verified Actions:

```text
ESP32 Build #1245 — GREEN
run 32515224487
head_sha 5fa6bcea812c33f0b2dc8e13baae476221839b3a

CMP Protocol Tests #2210 — GREEN
run 32515361340
head_sha ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133
```

После `5fa6bcea` до `ba3ac4bb` менялись только regression tests и documentation. Checkpoint 62 repo-level gates закрыты.

## Checkpoint 62 — закрытый hardening block

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

Verified semantics:

- lost ACK / `TIMED_OUT` never proves Arduino idle;
- ordinary dismiss cannot close `TIMED_OUT`;
- positive cancel closes only zero-run waiting state;
- stale cancel after `ProgramCompleted`/`ClosedAfterReview` is a no-op on persisted run evidence;
- final repeat target is checked before journal append;
- `RUN_STARTED` cannot reopen an already completed target;
- `RUN_COMPLETED` above immutable target is rejected before NDJSON mutation;
- deep session audit validates `JobRuntimeState.completedRuns` against immutable snapshot `repeatTarget` without another full journal scan.

Не возвращаться к этому блоку без конкретной регрессии.

## Закрытые блоки — не поднимать без регрессии

Не считать следующей работой:

- JOB cancel/recovery (`ALREADY_CLEAR`, `ALL_CLEAR`, physical fallback, timeout hardening);
- repeat-target model/UI/final-boundary implementation;
- Arduino autonomous archive UI/provenance;
- motor schema/detail/repair-history contracts;
- KG_FIRST quantity/writeoff/costing compatibility;
- writeoff fault-path hardening;
- NDJSON observability/rotation groundwork;
- warehouse/winding bounded scan hardening;
- backup whitelist/deep integrity/session preflight consolidation;
- web regression-contract recovery;
- obsolete duplicate ESP32 job-lifecycle implementation.

Если старый checkpoint говорит `next`, но этот раздел помечает блок закрытым, считать старый `next` историческим.

## Текущая активная очередь

### 1. Targeted two-board UART/hardware smoke

При доступном стенде проверить current checkpoint 62 boundary:

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

Это verification gate, а не новая разработка.

### 2. Дальнейшая работа

Если hardware smoke сейчас недоступен, не простаивать и не поднимать старые задачи. Выбирать следующую repo-level работу только из:

- конкретного bug report;
- подтверждённого persistence/integrity fault;
- измеренного performance/storage hotspot;
- явно запрошенной новой функциональности.

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
