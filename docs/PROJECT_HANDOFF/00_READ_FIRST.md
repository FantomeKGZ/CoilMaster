# CoilMaster — current project entrypoint

Дата обновления: **2026-08-22**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Что читать новому AI / coding agent

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Старые numbered checkpoints — history/evidence, а не backlog. Не продолжать старую задачу только потому, что исторический checkpoint содержит `next`/`pending`.

Перед изменением existing file обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить exact path. Не утверждать CI/build/hardware GREEN без фактического результата или явного подтверждения оператора.

## Current verification baseline

Последний явно подтверждённый пользователем GREEN state:

```text
3ebc942f1be9397af9d8ee5336c0ed78e9b13c87
Record calculator standard alternatives feature
USER CONFIRMED GREEN
```

Коммиты после этого SHA, включая заключительный full-audit пакет и network JSON hardening, требуют нового applicable workflow result или явного подтверждения пользователя. Пустой GitHub status response не считается GREEN.

## Current active phase

Full code audit A..E завершён на repo-review/source-contract уровне:

```text
A Arduino safety/realtime/UART
B ESP32 runtime/API/persistence/integrity/network/backup
C desktop/mobile/shared Web parity/error/security
D tests/CI/build filters/triggers
E docs/AI routing consistency
```

Authoritative audit checkpoint:

```text
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
```

До начала cleanup нужен final applicable CI gate для текущего post-`3ebc942f...` набора. После него пользователь уже одобрил отдельную repository cleanup/de-duplication phase.

## Safety invariants

Never weaken:

- physical START only physical/local;
- no automatic physical START between repeat cycles;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK / timeout never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never performs automatic wire/material writeoff;
- manual writeoff requires exact `source_session_id + source_run_id`;
- `spool_id` optional only in approved KG_FIRST unallocated/manual path;
- exact spool provenance retained whenever a spool is used;
- operational cancellation does not erase immutable run/history evidence;
- backup restore operator-only, transactional and fail-closed;
- no automatic production-data deletion.

## Production architecture

```text
ESP32: service/data/UI orchestration, SD/RTC/network, registry, jobs,
       persistence, warehouse/material/costing, backup/restore

CMP1 UART: JOB/control down, run/status events up

Arduino Uno: physical START, SSR, Hall, keypad/LCD/buzzer,
             realtime winding machine and RUN event generation
```

Production flow:

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> immutable snapshot/material selection -> UART JOB
-> physical START -> RUN_STARTED/RUN_COMPLETED
-> explicit manual exact-run writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

## External hardware gate

Targeted two-board ESP32<->Arduino UART/repeat/cancel smoke remains required when the physical stand is available. It is an external verification gate, not a repo-review backlog item. Hardware GREEN is never inferred from CI.

## Cleanup phase rule

After current applicable CI is GREEN, build a dependency inventory before deleting anything and classify every candidate as DELETE / MERGE / KEEP / REVIEW. Already known candidates include legacy conductor settings ownership, the obsolete calculator multisource helper and one-byte placeholder README files, but nothing is deleted solely because it looks old.
