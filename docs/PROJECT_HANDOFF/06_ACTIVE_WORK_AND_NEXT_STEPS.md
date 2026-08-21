# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — история/evidence, а не backlog.

## Current verified repo baseline

Последний явно подтверждённый repo-level baseline до текущего Phase 9 блока:

```text
ESP32 Build #1245 — GREEN
run 32515224487
head_sha 5fa6bcea812c33f0b2dc8e13baae476221839b3a

CMP Protocol Tests #2210 — GREEN
run 32515361340
head_sha ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133
```

Эти результаты **не** являются доказательством green для более новых Phase 9 commits.

Текущий Phase 9 implementation checkpoint:

```text
docs/PROJECT_HANDOFF/64_PHASE9_SHARED_WEB_SHELL_AND_SEARCH_2026-08-22.md
```

Phase 9 implementation: **COMPLETE**.  
Текущий HEAD CI: **NOT VERIFIED**, пока не получен exact Actions result с совпадающим `head_sha`.

## Активный приоритет — вернуться к отложенному FULL CODE AUDIT

Утверждённый план 2026-08-20 теперь закрыт на уровне реализации, включая Phase 9 Web UX core:

- unified desktop/mobile navigation;
- FTP settings shared shell;
- device clock без request каждую секунду;
- firmware/web build identification;
- shared toast/error handling;
- real global motor/client/repair search;
- recent items + breadcrumbs;
- shared shell contract audit.

Поэтому активная работа снова переходит в:

```text
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
```

### Следующий конкретный кодовый пункт

Продолжить Arduino audit с открытой находки **A-002**:

```text
P2 — production JOB parser accepts non-canonical tokens
```

Проверить и при необходимости исправить `Arduino/CM_UartEventTransport.cpp::parseRemoteJob()`:

- строгий canonical uint parsing;
- reject trailing characters/overflow/empty tokens;
- reject unknown winding type вместо неявного превращения в `WORKING`;
- fail-closed CRC-valid malformed JOB;
- сохранить backward compatibility только там, где она явно предусмотрена CMP protocol;
- добавить regression coverage.

Перед любым изменением fetch current file + current blob SHA.

## После A-002 — targeted UART desync/recovery audit

Не переизобретать уже реализованный JOB cancel/recovery. Проверить текущую реализацию на конкретные state-machine несогласованности и lost-ACK/reboot edge cases:

```text
JOB
JOB_ACK
JOB_CANCEL
JOB_CANCEL_ACK
ALREADY_CLEAR
ALL_CLEAR
timeout
late ACK / late ALL_CLEAR
ESP32 reboot
Arduino reboot
replay
persisted ESP32 job state
```

Особенно воспроизвести/проанализировать сценарий:

```text
ESP32 считает JOB отправленным/ожидаемым
Arduino задания не показывает
обычная отмена не восстанавливает согласованное состояние
```

Цель — доказать корректный recovery либо найти точный дефект. Исправлять только подтверждённый дефект; не ослаблять fail-closed правила.

## Дальнейшая очередь checkpoint 63

После Arduino + targeted UART audit:

1. ESP32 runtime/API/persistence/integrity/network/backup audit;
2. desktop/mobile Web/API/error/security parity audit;
3. tests/CI/build filters/path triggers/false-positive audit;
4. docs/AI routing consistency audit;
5. final cross-layer recheck и свежие applicable CI gates.

Приоритет находок:

```text
P0 physical/data safety or destructive corruption
P1 serious functional/state/persistence defect
P2 concrete robustness/performance/maintainability weakness
P3 low-risk cleanup/dead code/docs/test-quality issue
```

Не считать style preference или speculative redesign дефектом.

## Закрытая production-функциональность — не возвращать автоматически в backlog

Без конкретного defect/regression не переоткрывать:

- generic JOB cancel/recovery (`ALREADY_CLEAR`, `ALL_CLEAR`, timeout/manual-review hardening);
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

Перед текущей работой читать:

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/64_PHASE9_SHARED_WEB_SHELL_AND_SEARCH_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/62_JOB_LIFECYCLE_SAFETY_HARDENING_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Checkpoint 40 теперь исторический утверждённый план; checkpoint 64 фиксирует завершение его Phase 9 implementation. Старые numbered checkpoints читать только для исторических деталей конкретной реализации.
