# CoilMaster — current project entrypoint

Дата обновления: **2026-08-21**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Что читать новому AI / coding agent

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Старые numbered checkpoints — **история**, а не очередь активных работ. Не продолжать старую задачу только потому, что в checkpoint 24, 38 или другом старом файле написано `next`/`pending`. Активная работа определяется этим файлом, checkpoint `61`, текущим кодом/CI failure или прямой просьбой пользователя.

Перед изменением existing file обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить exact path. Не утверждать CI/build/hardware GREEN без фактического результата.

## Текущий подтверждённый repo baseline

Production commit:

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
Fix duplicate ESP32 job lifecycle definitions
```

На этом exact commit подтверждены:

```text
CMP Protocol Tests #2175 — GREEN
run 32499582610

ESP32 Build #1241 — GREEN
run 32499582503
```

Protocol/Web/safety audits и ESP32 compile/link сейчас не являются blocker.

## JOB cancel/recovery — реализовано и закрыто

Не считать этот блок текущей задачей без конкретной регрессии.

Реализовано:

- no-run pending/accepted remote JOB может быть safely cancelled;
- Arduino cancel идемпотентна для already-clear state (`ALREADY_CLEAR`);
- physical fallback `D -> * -> # -> D` отправляет `ALL_CLEAR` только когда нет active physical run;
- `ALL_CLEAR` не означает `RUN_COMPLETED`;
- reboot recovery не создаёт auto-start, fake completion или automatic wire writeoff;
- operational cancellation не должна стирать immutable run/history evidence.

Историческая реализация описана в checkpoint `39_JOB_CANCEL_RECOVERY_2026-08-18.md`; текущий статус зафиксирован в checkpoint `61`.

## Safety-инварианты

Нельзя менять:

- physical START только физический/local;
- никакого automatic physical START между repeat cycles;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
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

Checkpoints `46–57` — исторические implementation/hardening records, не active tasks.

## Backup/session integrity — реализовано

`WindingSessionPersistenceIntegrityAudit` является authoritative read-only preflight owner для session snapshot/state/spool-selection storage. Backup manifest использует его classified/measured results и не выполняет прежний duplicate full session preflight scan.

Checkpoints `58–59` — закрытый recovery block.

## Verification status

Подтверждено на `e35c4bfe`:

```text
CMP Protocol Tests — GREEN
ESP32 Build — GREEN
```

Отдельно не считать автоматически подтверждёнными:

```text
Arduino Uno Build current HEAD
current-head two-board UART hardware smoke
current-head full hardware acceptance
```

Старые hardware confirmations остаются историческим evidence соответствующего baseline, но не превращаются автоматически в доказательство нового production HEAD после затронувших scope изменений.

## Текущая точка продолжения

Repo-level recovery Protocol + ESP32 build закрыт.

Следующие действия выбирать только по текущему evidence:

```text
1. проверить Arduino Uno Build текущего HEAD;
2. при наличии железа выполнить targeted ESP32 <-> Arduino UART/hardware smoke для текущего HEAD;
3. исправлять только concrete failures/regressions;
4. performance/storage work делать по measurements, не по старым TODO;
5. новые product features брать из прямой текущей задачи пользователя.
```

Не возвращаться автоматически к уже закрытым pagination/archive/backup/KG_FIRST/JOB-cancel работам.

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

Если тематический документ конфликтует с current code или checkpoint `61`, приоритет у current code + фактического verification result + этого entrypoint.
