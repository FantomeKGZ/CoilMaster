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

Пользователь после documentation cleanup сообщил, что workflows зелёные, но новый production change ниже требует своих exact automated results перед присвоением GREEN.

## Current production candidate

```text
ce38711ca9ecd21fa3432e0981f0933878ac85dd
Keep timed-out jobs in manual review
```

Исправлен latent safety defect в `CM_JobStateDismiss.cpp`: ordinary `dismissInactive()` больше не считает `TIMED_OUT` terminal delivery. Потеря всех `JOB_ACK` не доказывает, что Arduino не приняла и не удерживает удалённый JOB, поэтому timeout закрывается только через explicit manual-review path.

Regression coverage:

```text
Tests/Web/check_job_cancel_recovery_contracts.js
fa5a0073ee80c58a1d1cab67d50a1249b9479d00  Guard timed-out job manual review
```

Новый audit также защищает lost-ACK -> JOB_CANCEL, ALREADY_CLEAR, physical ALL_CLEAR, no-run persisted closure и recovery re-evaluation.

Current candidate verification required:

```text
ESP32 Build — NOT VERIFIED for ce38711c/fa5a0073
CMP Protocol Tests — NOT VERIFIED for ce38711c/fa5a0073
```

Arduino production code/wire format этим изменением не менялись.

## Закрытые блоки — не поднимать без регрессии

Не считать следующей работой:

- JOB cancel/recovery (`ALREADY_CLEAR`, `ALL_CLEAR`, physical fallback), кроме проверки текущего timeout fix;
- repeat-target semantics и physical START gating;
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

### 1. Verify timeout manual-review fix

Получить exact results текущего production candidate:

```text
ESP32 Build
CMP Protocol Tests
```

Если красный — исправлять только фактическую ошибку/log regression.

### 2. Targeted two-board UART/hardware smoke

После green automated gates, при доступном стенде, проверить current boundary:

```text
ESP32 JOB delivery
-> Arduino receives/shows READY job
-> physical START only
-> RUN_STARTED
-> RUN_COMPLETED
-> event ACK/replay
-> no automatic material writeoff
```

Cancel/recovery smoke как targeted confirmation:

```text
no-run JOB -> cancel
already-clear cancel
physical D -> * -> # -> D fallback
lost JOB_ACK / timeout -> manual review, not ordinary dismiss/new-job bypass
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
docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Исторические checkpoints читать только для деталей уже выполненной реализации или evidence конкретного старого baseline.
