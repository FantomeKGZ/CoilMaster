# CoilMaster — current project entrypoint

Дата обновления: **2026-08-22**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Что читать новому AI / coding agent

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/62_JOB_LIFECYCLE_SAFETY_HARDENING_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Checkpoint `61` и более старые numbered checkpoints — история/evidence, а не очередь активных работ. Не продолжать старую задачу только потому, что в checkpoint 24, 38, 61 или другом старом файле написано `next`/`pending`.

Перед изменением existing file обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить exact path. Не утверждать CI/build/hardware GREEN без фактического результата.

## Last exact verified production baseline

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
Fix duplicate ESP32 job lifecycle definitions

CMP Protocol Tests #2175 — GREEN
run 32499582610

ESP32 Build #1241 — GREEN
run 32499582503
```

Пользователь позже сообщил, что workflows зелёные после documentation cleanup, но current production candidate из checkpoint 62 содержит новые ESP32 safety changes и требует своих exact workflow results.

## Current production candidate — checkpoint 62

Production hardening:

```text
ce38711c  Keep timed-out jobs in manual review
2ecebb5e  Ignore stale cancel after completed job
9e68bd86  Reject runs beyond immutable repeat target
```

Regression guards:

```text
fa5a0073  Guard timed-out job manual review
44d1e037  Guard stale cancel after completed job
5188084d  Guard immutable repeat target journal boundary
```

Current candidate verification label until actual runs are inspected:

```text
ESP32 Build — NOT VERIFIED
CMP Protocol Tests — NOT VERIFIED
hardware UART/repeat/cancel smoke — NOT VERIFIED
```

## JOB lifecycle semantics

Implemented/current candidate:

- if a JOB may have reached Arduino, ESP32 cancellation uses remote `JOB_CANCEL`, not local-only closure;
- Arduino `ALREADY_CLEAR` is idempotent success;
- physical fallback `D -> * -> # -> D` emits CRC-protected `ALL_CLEAR` only when safe;
- `ALL_CLEAR` never means `RUN_COMPLETED`;
- positive remote cancellation can persist closure only with zero physical-run evidence;
- `TIMED_OUT` is ambiguous and remains manual-review-only;
- ordinary `dismissInactive()` cannot close `TIMED_OUT`;
- stale duplicate cancel after `ProgramCompleted`/`ClosedAfterReview` is a persisted-state no-op;
- immutable `repeat_target` is checked before winding journal append;
- `RUN_STARTED` cannot reopen an already completed target;
- `RUN_COMPLETED` beyond immutable repeat target is rejected before NDJSON mutation;
- reboot never auto-starts, auto-completes or auto-writes off material.

See checkpoint `62` for exact files/commits.

## Safety-инварианты

Нельзя менять:

- physical START только физический/local;
- никакого automatic physical START между repeat cycles;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
- lost ACK / timeout never proves Arduino idle;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- manual material writeoff требует exact `source_session_id + source_run_id`;
- `spool_id` optional только в утверждённом KG_FIRST unallocated/manual path;
- если spool используется, exact spool provenance сохраняется;
- linked immutable history не удаляется operational cancellation;
- backup restore operator-only, transactional, fail-closed;
- hardware settings/calibration только при доказанном safe idle;
- Hall calibration запускает двигатель только через physical START и не создаёт production RUN history/writeoff.

## Реализованный production flow

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> immutable job snapshot/material selection
-> UART JOB delivery -> physical START
-> RUN_STARTED / RUN_COMPLETED
-> explicit manual exact-run material writeoff
-> costing / finalization -> CLOSED -> reports -> backup
```

Repeat semantics:

```text
program + repeat_target = one JOB
one complete program pass = one RUN / one run_id
new physical START required for every repeat
```

Automatic repeat START отсутствует.

## KG_FIRST material semantics — реализовано

```text
quantity_kg authoritative for new consumption
source_session_id + source_run_id mandatory
spool_id optional only for approved unallocated/manual KG_FIRST consumption
spool decrement only when exact spool is present
legacy exact-spool records remain compatible
RUN_COMPLETED never auto-deducts material
```

Checkpoints `46–57` — historical implementation/hardening evidence, not active tasks.

## Backup/session integrity — реализовано

`WindingSessionPersistenceIntegrityAudit` является authoritative read-only preflight owner для session snapshot/state/spool-selection storage. Backup manifest использует его classified/measured results и не выполняет прежний duplicate full session preflight scan.

Checkpoints `58–59` — закрытый recovery block.

## Текущая точка продолжения

```text
1. подтвердить current-candidate ESP32 Build + CMP Protocol Tests;
2. если green — targeted ESP32 <-> Arduino UART/repeat/cancel smoke при доступном стенде;
3. исправлять только concrete failures/regressions;
4. performance/storage work делать по measurements;
5. новые product features брать из прямой текущей задачи пользователя.
```

Не возвращаться автоматически к уже закрытым pagination/archive/backup/KG_FIRST работам.

## Основные тематические документы

```text
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/02_ARCHITECTURE_AND_HARDWARE.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/PROJECT_HANDOFF/04_DATA_STORAGE_API_UI.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/08_WORK_RULES_AND_VERIFICATION.md
docs/PROJECT_HANDOFF/09_KEY_FILES_INDEX.md
docs/HARDWARE_REFERENCE/
docs/AI_AGENT/
```

Если тематический документ конфликтует с current code или checkpoint `62`, приоритет у current code + фактического verification result + этого entrypoint.
