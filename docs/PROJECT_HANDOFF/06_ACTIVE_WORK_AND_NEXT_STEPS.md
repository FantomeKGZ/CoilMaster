# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — история/evidence, а не backlog.

## Current verified repo baseline

Последние connector-verified automated gates до текущего audit-блока:

```text
ESP32 Build #1245 — GREEN
run 32515224487
head_sha 5fa6bcea812c33f0b2dc8e13baae476221839b3a

CMP Protocol Tests #2210 — GREEN
run 32515361340
head_sha ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133
```

Оператор сообщил, что кроме перечисленных им красных ESP32 runs остальные видимые workflows зелёные. Это **USER CONFIRMED**, а не connector-verified для каждого более нового SHA.

## Текущий CI recovery checkpoint

Семь последовательных красных ESP32 runs #1263–#1269 исследованы и сведены к одному inherited Phase 9 build-identity compile-break. Подробности и exact run IDs:

```text
docs/PROJECT_HANDOFF/66_ESP32_BUILD_ID_CI_RECOVERY_2026-08-22.md
```

Исправления:

```text
c07c188a2a429cd68cb7fc8d1925e90a5d789cc9  Generate build identity header safely
3d63dc281d40070d90b42fbc7b3a202ab8f27f14  Guard build identity header generation
5fa8c89250c517f57ab61f24f3e61ebc98931234  Cover ESP32 build inputs in CI
```

Свежий `ESP32 Build` на потомке этих fixes должен быть подтверждён перед тем, как считать current audit code GREEN.

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
B-001 backup runtime Unavailable can no longer become Safe
B-003 network API validation/storage error semantics
```

B-003 code/tests:

```text
1048dd922d059b3f8ec0f31fafc3bd24795688af
5c05ad2f636ff5548ca80317010346dd66ad1af5
```

## Current open P1 — B-002 restore/apply production-mutation interlock

После подтверждения свежего ESP32 build продолжить именно B-002.

Подтверждённый race:

- restore apply/rollback идёт кусками в нескольких loop iterations;
- `webServer.handleClient()` способен принять новую production mutation между chunks;
- apply-loop изначально проверяет idle, но не имеет общего cross-layer mutation lock на весь период;
- local/physical Arduino activity также должна останавливать forward apply fail-closed.

Правильный fix должен:

1. блокировать новые production mutation HTTP requests на время apply/rollback;
2. не останавливать Arduino/UART polling;
3. повторно проверять runtime safety во время forward apply;
4. при потере proven-safe runtime прекращать forward apply и переходить в existing rollback;
5. не продолжать production-file rollback/copy, пока runtime снова не proven Safe;
6. не добавлять automatic START/resume/writeoff.

Не закрывать B-002 только JOB-specific проверкой.

## После B-002

Продолжить `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`, section B:

1. persistence/atomic-write/integrity partial-failure paths;
2. network/AP/STA/FTP/RTC/SD fail-closed behavior;
3. remaining backup/restore/activity-guard consistency;
4. NDJSON/resource hotspots только по evidence;
5. desktop/mobile Web/API/error/security parity;
6. tests/CI/build filter/false-positive audit;
7. docs/AI routing consistency;
8. final cross-layer recheck + exact applicable CI.

## Closed production work — не возвращать автоматически

Без concrete defect/regression не переоткрывать:

- generic JOB cancel/recovery;
- repeat-target/final-repeat lifecycle;
- Arduino autonomous archive UI/provenance;
- motor schema/detail/repair-history contracts;
- KG_FIRST quantity/writeoff/costing compatibility;
- writeoff fault-path hardening;
- warehouse/winding bounded scan hardening;
- backup whitelist/deep integrity/session preflight;
- Phase 9 shared Web shell/search implementation;
- ESP32 duplicate lifecycle linker recovery.

## External hardware verification gate

Targeted two-board ESP32<->Arduino smoke остаётся обязательным при доступном стенде, но не блокирует repo-level audit.

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
USER CONFIRMED user explicitly verified visible workflow/hardware state
```

Git commit сам по себе не является build result.

## Read order for continuation

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/66_ESP32_BUILD_ID_CI_RECOVERY_2026-08-22.md
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/64_PHASE9_SHARED_WEB_SHELL_AND_SEARCH_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```
