# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Verification baseline

Последние старые connector-verified gates остаются в checkpoint 63/66. После CI-recovery пользователь явно сообщил: **«Все зелёное»**. Это **USER CONFIRMED GREEN** для состояния ветки непосредственно перед текущими B-002+ commits, но не доказательство для более новых SHA.

Текущий post-persistence candidate: **NOT VERIFIED in chat**.

## Закрыто в full-code audit — не возвращать без concrete regression

```text
A-001..A-007 JOB/UART safety/recovery findings
B-001 backup runtime Unavailable -> fail closed
B-002 global restore/apply production-mutation interlock
B-003 network API validation/storage HTTP semantics
B-004 recoverable JobStateStore atomic replacement
B-006 committed-first NetworkProfileStore recovery
```

Подробные причины/commits/tests находятся в:

```text
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/66_ESP32_BUILD_ID_CI_RECOVERY_2026-08-22.md
```

## Current open finding — B-005 P2

Linked JOB preparation сейчас идёт:

```text
allocate IDs
snapshot
exact spool selection
runtime state
UART queue
```

Если exact spool selection записан, а runtime-state commit затем падает, может остаться orphan selection. Deep session audit такой selection без state отвергает. Простое удаление snapshot/selection на любой state error запрещено: state write может вернуть failure после фактического появления committed target, и тогда cleanup уничтожит provenance.

Следующий coding target: решить B-005 только через provenance-safe transaction semantics — explicit preparation/commit marker либо точную post-failure проверку state identity + narrowly scoped cleanup для доказанно uncommitted preparation.

## После B-005

Продолжить section B:

1. remaining backup/restore/activity-guard consistency;
2. remaining mutable persistence stores на destructive swap / ambiguous recovery;
3. resource/NDJSON hotspots только по evidence.

Если concrete section-B defects закончились — перейти к:

4. desktop/mobile Web/API/error/security parity;
5. tests/CI/build-filter/false-positive audit;
6. docs/AI routing consistency;
7. final cross-layer recheck + exact applicable CI.

## Уже просмотрено без нового production-data defect в текущем pass

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

restore interlock:
GET status remains available during APPLY
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
