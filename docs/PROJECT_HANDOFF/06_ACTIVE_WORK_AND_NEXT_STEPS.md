# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Verification baseline

Пользователь явно сообщил **«Все обновления зелёные»** для состояния ветки:

```text
3ebc942f1be9397af9d8ee5336c0ed78e9b13c87
Record calculator standard alternatives feature
```

Это **USER CONFIRMED GREEN** для калькулятора с одной строкой до 5 исходных проводов, отдельными warehouse/standard recommendations и всего предыдущего audit-набора. Любые commits после этого SHA требуют нового подтверждения или exact workflow result.

## Калькулятор — текущий production contract

```text
source input:
одна строка до 5 проводов
пример: 0,51;0,71;0,95
каждое значение = одна исходная жила

output block 1:
По складу — диаметры, уже известные warehouse catalogue

output block 2:
Стандартные варианты — read-only IEC 60317 R20 reference catalogue,
включая диаметры, которых никогда не было на складе
```

Backend `/api/calculator/conductor` возвращает `recommendations` и `standard_recommendations`. Стандартный каталог не создаёт spool, не изменяет warehouse и не участвует в writeoff/provenance.

Текущая модель диаметра CoilMaster — `diameterHundredthsMm`, точность **0,01 мм**. Реальный IEC R20 ряд адаптирован к этой точности; переход на 0,001 мм делать только отдельной явной миграцией модели/данных.

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
B-008 committed-first legacy ConductorSettingsStore recovery
B-009 production /data/settings/conductor.json atomic transaction
B-010 verified warehouse spool swap / rollback
B-011 verified material ledger swap / rollback
```

Для spool/material replacement rename сам по себе не считается commit-proof: temp и новый authoritative main проходят parser-backed validation, `.bak` удаляется только после проверки нового main, damaged promoted main откатывается на последний valid backup.

Прямой append при создании новой spool/material записи остаётся отдельным **P2 crash-resilience finding**: partial tail корректно fail-closed, но может сделать весь catalogue unavailable до operator recovery. Не лечить автоматическим truncation — автоматическое удаление production evidence запрещено.

## Web/API audit — текущие результаты

Проверены shared app-shell, Wi-Fi settings, Arduino archive, winding-history spool metadata, pricing history и writeoff UI. В просмотренных местах user/server strings либо проходят HTML escaping, либо выводятся через `textContent`.

Открытый P2:

```text
network JSON escaping
```

`CM_NetworkWeb.cpp` escape helper экранирует quote/backslash, но не все JSON control bytes. Старый `/api/system/network` в `CM_StaticSiteServer.cpp` вставляет STA SSID без JSON escaping вообще. Исправить через безопасный owner/refactor, не делать слепой full-file replace большого StaticSiteServer.

Подтверждённый cleanup-кандидат, пока НЕ удалять:

```text
firmware/esp32/web/shared/calculator-multisource.js
```

`CM_StaticSiteServer` всё ещё загружает helper на calculator page, но новый calculator больше не имеет legacy `#diameter/#strands`; helper сразу выходит и дублирует уже встроенный новый UI contract. Удаление/инъекцию чистить только в post-audit cleanup phase после dependency check.

## Tests/CI audit — сделано после GREEN baseline, verification pending

Добавлено:

```text
Tests/Web/check_material_ledger_atomic_recovery.js
```

Он защищает B-011 material-ledger swap/rollback contract и подключён к `CMP Protocol Tests`.

Исправлены workflow trigger gaps:

```text
Arduino Uno Build: Shared/** теперь запускает UNO compile
CMP Protocol Tests PR: Shared/** вместо только Shared/Protocol/**
```

Это необходимо, потому что production Arduino transport включает `Shared/CMP1Text/CM_Cmp1Crc.h`.

Добавлен `Tests/Web/check_ci_trigger_contracts.js`, защищающий критические build/test path filters и production environments.

`motor-reference.yml` проверен: checkout/build/push жёстко работают с `cmp-protocol-v1`; `main` источником не используется.

## Docs/AI routing audit — сделано после GREEN baseline, verification pending

Исправлены:

```text
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
```

Убрана stale routing-ссылка на checkpoint 61, active work теперь checkpoint 63 + этот файл 06. Добавлен approved post-audit cleanup stage и явный authoritative owner conductor calculator settings:

```text
CM_ConductorSettingsWeb.*
WarehouseStore::loadConversionSettings / setConversionSettings
/data/settings/conductor.json
```

Legacy `CM_ConductorSettings.*` не является production owner и остаётся cleanup candidate до dependency proof.

## Current active queue — сначала завершить полный аудит

1. закрыть Web/API JSON escaping finding и завершить desktop/mobile/shared parity/security review;
2. закончить tests/CI false-positive/orphan-test audit;
3. проверить `docs/AI_AGENT/04_VERIFICATION_MATRIX.md` и оставшиеся AI/docs entrypoints;
4. final cross-layer recheck production flow + safety invariants;
5. получить applicable GREEN для commits после `3ebc942f...`.

## После завершения аудита — отдельная cleanup phase

Пользователь явно одобрил очистку проекта после полного аудита. До удаления построить repo-wide dependency inventory и классифицировать каждый кандидат:

```text
DELETE  — доказанно не используется production/build/tests/docs/runtime
MERGE   — дублирует другой authoritative implementation
KEEP    — нужен production/build/tests/docs/history/operator flow
REVIEW  — зависимость не доказана; не удалять
```

Из уже подтверждённых кандидатов:

```text
CM_ConductorSettings.cpp/.h               legacy parallel persistence owner
shared/calculator-multisource.js          legacy calculator UI helper
.github/workflows/README.md                1-byte placeholder
.github/ISSUE_TEMPLATE/README.md           ранее замеченный placeholder; перепроверить перед delete
```

Не удалять ничего только потому, что имя выглядит старым. Проверять includes/imports, PlatformIO build, workflow/test references, static script injection, runtime file paths, migrations/history and AI/docs routing. После cleanup — полный applicable CI и сравнение дерева до/после.

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
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/66_ESP32_BUILD_ID_CI_RECOVERY_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```
