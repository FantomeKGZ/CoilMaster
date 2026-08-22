# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — история/evidence, а не backlog.

## Current verified repo baseline

Последние явно подтверждённые automated gates до текущего audit-блока:

```text
ESP32 Build #1245 — GREEN
run 32515224487
head_sha 5fa6bcea812c33f0b2dc8e13baae476221839b3a

CMP Protocol Tests #2210 — GREEN
run 32515361340
head_sha ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133
```

Эти результаты **не** являются доказательством green для более новых audit commits.

Текущий UART/recovery implementation checkpoint:

```text
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
```

Implementation HEAD, зафиксированный в checkpoint 65:

```text
bf3ac8c18a3d8b484eaf755452965b670973f627
```

Точный CI этого code block: **NOT VERIFIED**.

## Что закрыто в текущем full-code audit

Не возвращать автоматически в очередь:

```text
A-001 remote JOB admission hardening
A-002 strict Arduino JOB parser
A-003 strict ACK/NACK + JOB_CANCEL correlation IDs
A-004 recovery-only zero-id ALL_CLEAR identity
A-005 stale cancel/run-evidence storage-health handling
A-006 control-result-before-RUN ordering barriers
A-007 lost JOB_ACK timeout -> exact late RUN_STARTED reconciliation
```

Подробности находятся в checkpoints 63 и 65. Эти пункты остаются с exact-current CI/hardware verification pending, но их repo-level реализация уже не является следующим coding task.

## Активный приоритет — ESP32 runtime/API/persistence/integrity/network/backup audit

Продолжить `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`, section B.

### Следующий конкретный review block

Проверить текущие `firmware/esp32/src/` на подтверждённые дефекты в:

1. runtime/API lifecycle transitions и HTTP/error semantics;
2. persistence/atomic-write/integrity partial-failure paths;
3. network/AP/STA/FTP/RTC/SD fail-closed behavior;
4. backup/restore/activity-guard consistency;
5. NDJSON/resource hot paths только по фактическим evidence, без преждевременной миграции в БД.

Приоритет — сначала state/data safety и реальные функциональные дефекты, затем performance/maintainability.

## После ESP32 audit

1. desktop/mobile Web/API/error/security parity audit;
2. tests/CI/build filters/path triggers/false-positive audit;
3. docs/AI routing consistency audit;
4. final cross-layer recheck;
5. свежие applicable CI gates на точных SHA.

## Закрытая production-функциональность — не возвращать автоматически в backlog

Без конкретного defect/regression не переоткрывать:

- generic JOB cancel/recovery;
- repeat-target/final-repeat lifecycle;
- Arduino autonomous archive UI/provenance;
- motor schema/detail/repair-history contracts;
- KG_FIRST quantity/writeoff/costing compatibility;
- writeoff fault-path hardening;
- NDJSON observability/rotation groundwork;
- warehouse/winding bounded scan hardening;
- backup whitelist/deep integrity/session preflight;
- web regression-contract recovery;
- ESP32 duplicate lifecycle linker recovery;
- Phase 9 shared Web shell/search implementation.

## External hardware verification gate

Targeted two-board ESP32<->Arduino smoke остаётся обязательным при доступном стенде, но это external verification gate и не блокирует repo-level аудит.

Проверить:

```text
normal JOB -> Arduino READY
physical START only
RUN_STARTED -> RUN_COMPLETED
repeat > 1 -> physical START each run
event ACK/replay
no automatic material writeoff

zero-run JOB -> cancel
ALREADY_CLEAR
safe physical ALL_CLEAR
late zero-id ALL_CLEAR must not cancel fresh job
lost JOB_ACK -> timeout/manual review -> late RUN_STARTED reconciliation
reboot in waiting/running state -> no auto resume
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
- manual writeoff requires exact source session/run provenance;
- KG_FIRST may omit spool only in approved unallocated/manual path;
- exact spool provenance retained when a spool is used;
- backup/restore remains explicit, operator-only and fail-closed;
- no automatic production-data deletion.

## Verification language

```text
GREEN/SUCCESS  named workflow/test actually completed successfully
FAILED         named gate actually ran and failed
NOT VERIFIED   result unavailable/not run
USER CONFIRMED user explicitly verified real hardware behavior
```

Git commit сам по себе не является build result.

## Документация для продолжения

Перед текущей работой читать:

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/64_PHASE9_SHARED_WEB_SHELL_AND_SEARCH_2026-08-22.md
docs/PROJECT_HANDOFF/62_JOB_LIFECYCLE_SAFETY_HARDENING_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Checkpoint 40 — исторический утверждённый план; checkpoint 64 — завершение Phase 9; checkpoint 65 — текущий UART/desync recovery audit evidence. Активная очередь находится только здесь, в `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
