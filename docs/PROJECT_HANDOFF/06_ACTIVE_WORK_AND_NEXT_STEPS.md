# Активная работа и следующие шаги

Дата обновления: **2026-08-21**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints сохраняются как история и **не являются backlog**.

## Current verified baseline

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

Repo-level Protocol/Web/safety recovery и ESP32 compile/link recovery закрыты.

## Закрытые блоки — не поднимать без регрессии

Не считать следующей работой:

- JOB cancel/recovery (`ALREADY_CLEAR`, `ALL_CLEAR`, physical fallback);
- repeat-target semantics и physical START gating;
- Arduino autonomous archive UI/provenance;
- motor schema/detail/repair-history contracts;
- KG_FIRST quantity/writeoff/costing compatibility;
- writeoff fault-path hardening;
- NDJSON observability/rotation strategy groundwork;
- warehouse/winding bounded scan hardening;
- backup whitelist/deep integrity/session preflight consolidation;
- web regression-contract recovery, восстановленный до GREEN;
- ESP32 duplicate job-lifecycle linker regression, исправленный на `e35c4bfe`.

Если старый checkpoint говорит `next`, но этот раздел помечает блок закрытым, считать старый `next` историческим.

## JOB cancel/recovery status

Реализация проверена на уровне текущего source:

```text
no-run remote cancel
already-clear -> idempotent success
D -> * -> # -> D -> ALL_CLEAR when safe
active physical run -> clear rejected
ALL_CLEAR != RUN_COMPLETED
reboot != auto-start / auto-completion / auto-writeoff
```

Без конкретного воспроизводимого defect этот блок не трогать.

## Текущая активная очередь

### 1. Arduino Uno Build current HEAD

Нужно получить фактический результат `.github/workflows/arduino-uno-build.yml` или эквивалентного `pio run -e uno` для текущего производственного состояния. Старые Uno success/failure не заменяют current result.

Если build красный — исправлять только точную compile/link/Flash/SRAM причину и сохранять safety semantics.

### 2. Targeted two-board UART/hardware smoke

После green Uno build, при доступном стенде, проверить только current affected boundary:

```text
ESP32 JOB delivery
-> Arduino receives/shows READY job
-> physical START only
-> RUN_STARTED
-> RUN_COMPLETED
-> exact event ACK/replay behavior
-> no automatic material writeoff
```

Дополнительно короткий cancel smoke для уже реализованного recovery нужен только как targeted confirmation, а не как новая разработка:

```text
no-run JOB -> cancel
already-clear cancel
physical D -> * -> # -> D fallback
```

### 3. Дальнейшая работа

После current build/hardware gates выбирать задачу только из:

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
- RUN_COMPLETED never auto-writes off material;
- manual writeoff requires exact source session/run provenance;
- KG_FIRST may omit spool only in approved unallocated/manual path;
- exact spool provenance retained when a spool is used;
- backup/restore remains explicit, operator-only and fail-closed.

## Verification language

Использовать только точные статусы:

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
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Исторические checkpoints читать только для деталей уже выполненной реализации или evidence конкретного старого baseline.
