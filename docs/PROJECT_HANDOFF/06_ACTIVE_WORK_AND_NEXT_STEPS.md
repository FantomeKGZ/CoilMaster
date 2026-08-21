# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — история/evidence, а не backlog.

## Current verified repo baseline — checkpoint 62

Production ESP32 C++ baseline:

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

Checkpoint 62 repo-level gates закрыты.

## Активный приоритет — закончить утверждённый план 2026-08-20

Перед продолжением общего full-code audit сначала довести до 100% утверждённый план:

```text
docs/PROJECT_HANDOFF/40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md
```

Phase 1–8 считаются реализованными на repo level последующими checkpoints. Текущий незакрытый остаток — Phase 9 / Web UX core, включая проверку и при необходимости реализацию:

- unified desktop/mobile navigation;
- нормальный shell для FTP settings;
- global device clock без тяжёлого request каждую секунду;
- firmware/web version identification;
- shared toast/error handling;
- global search / recent items / breadcrumbs;
- UI contract tests для shell/nav/clock/internal desktop-mobile links.

План не считать полностью завершённым, пока каждый пункт Phase 9 не подтверждён текущим кодом/tests или явно не закрыт совместимой реализацией.

## После закрытия плана — ОБЯЗАТЕЛЬНО продолжить отложенную работу

Сразу после фиксации 100% завершения плана 2026-08-20 вернуться к работе, которую временно отложили ради последовательного завершения одного блока:

1. продолжить `FULL CODE AUDIT` по checkpoint 63;
2. продолжить targeted ESP32↔Arduino desync/recovery audit сценария, где JOB не появился на Arduino и затем не удаётся нормально очистить состояние;
3. отдельно пройти `JOB / JOB_CANCEL / ALL_CLEAR / timeout / reboot / recovery` transitions на обеих платах и в ESP32 persisted state;
4. исправлять только подтверждённые дефекты, не ослабляя safety-инварианты;
5. затем продолжить оставшиеся пункты checkpoint 63 по ESP32 runtime/API/persistence, Web parity, tests/CI и docs consistency.

Эта очередь **не отменена и не заменена Phase 9**. Она намеренно отложена до закрытия утверждённого плана, чтобы сначала закончить один блок полностью и только потом переходить дальше.

## План обновления — закрытая основная production-функциональность

Закрыты и не являются активной разработкой без конкретного defect/regression:

- JOB cancel/recovery (`ALREADY_CLEAR`, `ALL_CLEAR`, timeout/manual-review hardening);
- repeat-target/final-repeat lifecycle hardening;
- Arduino autonomous archive UI/provenance;
- motor schema/detail/repair-history contracts;
- KG_FIRST quantity/writeoff/costing compatibility;
- writeoff fault-path hardening;
- NDJSON observability/rotation groundwork;
- warehouse/winding bounded scan hardening;
- backup whitelist/deep integrity/session preflight consolidation;
- web regression-contract recovery;
- ESP32 compile/link lifecycle recovery.

## External hardware verification gate

Targeted two-board ESP32<->Arduino smoke остаётся обязательным при доступном стенде, но это **external verification gate**, а не текущий software backlog.

Проверить на железе:

```text
normal JOB -> Arduino READY
physical START only
RUN_STARTED -> RUN_COMPLETED
event ACK/replay
final repeat remains terminal
no automatic material writeoff

no-run JOB -> cancel
ALREADY_CLEAR
D -> * -> # -> D -> ALL_CLEAR when safe
lost JOB_ACK / timeout -> manual review
late ALL_CLEAR after completed job -> no persisted corruption/storage fault
```

Недоступность стенда не должна блокировать repo-level аудит.

## Deferred full-code audit — authoritative checkpoint 63

После завершения Phase 9 использовать:

```text
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
```

Порядок:

1. Arduino safety/realtime/UART/state-machine/resource audit;
2. ESP32 runtime/API/persistence/integrity/network/backup audit;
3. desktop/mobile web/API/error/security parity audit;
4. tests/CI/build filters/path triggers/false-positive audit;
5. docs/AI routing consistency audit;
6. final cross-layer recheck и свежие applicable CI gates.

Приоритет находок:

```text
P0 physical/data safety or destructive corruption
P1 serious functional/state/persistence defect
P2 concrete robustness/performance/maintainability weakness
P3 low-risk cleanup/dead code/docs/test-quality issue
```

Не считать style preference или speculative redesign дефектом.

## Safety boundary

Не менять:

- physical START only physical/local;
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

Пока Phase 9 не закрыт, перед работой читать:

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Checkpoint 62 — verified baseline/evidence. Более старые numbered checkpoints читать только для исторических деталей конкретной реализации.
