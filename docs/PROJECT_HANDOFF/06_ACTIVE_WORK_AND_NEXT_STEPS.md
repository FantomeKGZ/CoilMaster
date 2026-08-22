# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Verification baseline

Пользователь явно сообщил **«Все зелёное»** для состояния ветки:

```text
ef095a5eb05ae5f886020510ef11324d0f4882ad
Advance persistence audit past settings recovery
```

Это **USER CONFIRMED GREEN** для B-005/B-007/B-008 и всего предыдущего текущего набора. Любые commits после этого SHA требуют нового подтверждения.

## Закрыто в full-code audit — не возвращать без concrete regression

```text
A-001..A-007 JOB/UART safety/recovery findings
B-001 backup runtime Unavailable -> fail closed
B-002 global restore/apply production-mutation interlock
B-003 network API validation/storage HTTP semantics
B-004 recoverable JobStateStore atomic replacement
B-005 provenance-safe linked JOB preparation transaction
B-006 committed-first NetworkProfileStore recovery
B-007 committed-first RemoteBackupSettingsStore recovery
B-008 committed-first ConductorSettingsStore recovery
```

Общий crash-consistency rule для mutable settings stores:

```text
valid committed main -> keep main, cleanup residue
no valid main + valid backup -> restore backup, discard prepared temp
no backup + valid temp -> promote only as interrupted first write
invalid backup evidence -> fail closed
```

B-005 strict pre-UART order:

```text
snapshot
-> CREATED + WAITING_DELIVERY + zero-run state
-> exact spool selection
-> DELIVERING state
-> UART queueJob
```

## Current active queue — сначала полный аудит

1. закончить ESP32 persistence/backup/activity/resource audit;
2. полный desktop/mobile/shared Web/API/error/security parity audit;
3. полный tests/CI/build-filter/false-positive audit;
4. docs/AI routing consistency audit;
5. final cross-layer recheck + applicable CI.

## После завершения аудита — отдельная cleanup phase

Пользователь явно одобрил очистку проекта после полного аудита. До удаления построить repo-wide dependency inventory и классифицировать каждый кандидат:

```text
DELETE  — доказанно не используется production/build/tests/docs/runtime
MERGE   — дублирует другой authoritative implementation
KEEP    — нужен production/build/tests/docs/history/operator flow
REVIEW  — зависимость не доказана; не удалять
```

Cleanup scope:

- лишние/пустые папки и файлы;
- временные файлы и переходные artifacts;
- мёртвый код;
- устаревшие реализации, оставшиеся после переноса между чатами/архитектурных изменений;
- дубли классов, helpers, web assets, tests или docs;
- пустые placeholder-файлы, если они действительно не нужны GitHub/tooling.

Не удалять ничего только потому, что имя выглядит старым. Сначала проверять includes/imports, build manifests, workflow paths, web references, runtime file paths, docs/AI routing и tests. После cleanup — полный applicable CI и сравнение дерева до/после.

## Уже просмотрено без нового production-data defect

- PersistentIdAllocator transaction/high-water recovery;
- immutable JobSnapshotStore create path;
- immutable JobSpoolSelectionStore create/temp identity checks;
- append-only RepairRegistry integrity behavior;
- NetworkManager AP recovery sequencing;
- RTC safe-idle NTP write/verify path;
- WebRecoveryFtpServer activity guard and `/web`-only scope.

## External hardware verification gate

Targeted ESP32<->Arduino smoke остаётся обязательным при доступном стенде, но не блокирует repo-level audit:

```text
normal JOB -> Arduino READY
physical START only
RUN_STARTED -> RUN_COMPLETED
repeat > 1 -> physical START each run
event ACK/replay
no automatic material writeoff
zero-run cancel / ALREADY_CLEAR / safe ALL_CLEAR
late zero-id ALL_CLEAR must not cancel fresh job
lost JOB_ACK -> timeout/manual review -> late RUN_STARTED reconciliation
reboot waiting/running -> no auto resume

B-005 preparation:
failed linked preparation before DELIVERING -> no JOB on Arduino
next higher-ID job allowed after reboot
DELIVERING/reboot -> manual review remains required

restore interlock:
GET status available during APPLY
POST/DELETE /api/* -> 409 restore_mutation_active
APPLIED -> reboot required before mutations resume
```

## Safety boundary

Не менять:

- physical START only physical/local;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web do not drive SSR directly;
- timeout/lost ACK alone never proves Arduino idle;
- final repeat cannot reopen automatically;
- RUN_COMPLETED never auto-writes off material;
- manual writeoff exact source session/run provenance;
- KG_FIRST spool omission only approved unallocated/manual path;
- exact spool provenance when spool is used;
- backup/restore explicit, operator-only, transactional/fail-closed;
- no automatic production-data deletion.

## Verification language

```text
GREEN/SUCCESS  named workflow/test actually completed successfully
FAILED         named gate actually ran and failed
NOT VERIFIED   result unavailable/not run
USER CONFIRMED user explicitly verified visible workflow/hardware state
```

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/66_ESP32_BUILD_ID_CI_RECOVERY_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```