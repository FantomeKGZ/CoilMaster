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

Checkpoint `62` — текущий verified repo-level baseline. Более старые numbered checkpoints — история/evidence, а не очередь активных работ. Не продолжать старую задачу только потому, что старый checkpoint содержит `next`/`pending`.

Перед изменением existing file обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить exact path. Не утверждать CI/build/hardware GREEN без фактического результата.

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

Repo-level update/hardening plan through checkpoint 62 is closed.

## External hardware gate

Targeted two-board ESP32<->Arduino UART/repeat/cancel smoke remains required when the physical stand is available. It is an external verification gate, not an active software-development backlog item, and must not prevent repo-level audit work.

Hardware GREEN is not implied by CI.

## Current active phase — full code audit

The active task is now:

```text
FULL CODE AUDIT of cmp-protocol-v1
```

Authoritative audit checkpoint:

```text
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
```

Audit order:

```text
Arduino safety/realtime/UART/resources
ESP32 runtime/API/persistence/integrity/network/backup
Desktop/mobile web parity/error/security
Tests/CI/build filters/triggers
Docs/AI routing consistency
Final cross-layer recheck + applicable CI
```

Confirmed defects are fixed as they are found, using current blob SHA and the smallest safe change. Do not accumulate speculative redesigns.

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

## Implemented blocks that are not active backlog

Do not restart without a concrete regression:

- JOB cancel/recovery and timeout/manual-review hardening;
- repeat-target/final-repeat semantics;
- Arduino autonomous archive;
- motor schema/import/detail/repair history;
- bounded/paged growing APIs;
- network/AP/FTP foundations;
- backup/deep integrity/session preflight;
- KG_FIRST material/writeoff/costing;
- writeoff fault-path hardening;
- NDJSON observability groundwork;
- Hall settings/calibration safety;
- Protocol/Web/ESP32 build recovery.

## Main thematic docs

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

If a thematic or historical document conflicts with current code, actual verification evidence, this entrypoint, or checkpoint 63, prioritize current code + actual evidence + current entrypoints.
