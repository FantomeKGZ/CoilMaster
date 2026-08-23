# CoilMaster — current project entrypoint

Дата обновления: **2026-08-23**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Что читать новому AI / coding agent

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/68_REFERENCE_INTEGRATION_LOG_2026-08-23.md
docs/PROJECT_HANDOFF/64_RUNTIME_PROVENANCE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Старые numbered checkpoints — history/evidence, а не backlog. Не продолжать старую задачу только потому, что исторический checkpoint содержит `next`/`pending`.

Перед изменением/deletion existing file обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить exact path. Не утверждать CI/build/hardware GREEN без фактического результата или явного подтверждения оператора. Empty GitHub code-search не является достаточным доказательством отсутствия dependency.

## Current verification baseline

Software cleanup verification checkpoint завершён **2026-08-23**. Пользователь явно подтвердил, что все текущие GitHub Actions зелёные.

Последний implementation/test cleanup commit:

```text
bd64e3cc4ba92a6624aed677d98c1620c165013e
test(warehouse): guard against duplicate web bootstrap
```

Он защищает implementation fix:

```text
06a752663504d58ca6908414f8aa8786007c6877
fix(esp32): remove duplicate warehouse web bootstrap
```

Более поздние checkpoint commits — documentation/status synchronization.

## Current active phase

Software cleanup закрыт. Текущая активная продуктовая работа — интеграция legacy-справочника обмотчика из `FantomeKGZ/motor-winding-reference` с единым desktop/mobile дизайном CoilMaster, общими ресурсами и сохранением таблиц/изображений/описаний.

Подробный хронологический журнал этой работы:

```text
docs/PROJECT_HANDOFF/68_REFERENCE_INTEGRATION_LOG_2026-08-23.md
```

Его обновлять после каждого завершённого блока вместе с обычным handoff current-state.

Full code audit A..E завершён:

```text
A Arduino safety/realtime/UART/resources                  COMPLETE
B ESP32 runtime/API/persistence/integrity/network/backup COMPLETE
C desktop/mobile/shared Web parity/error/security        COMPLETE
D tests/CI/build filters/triggers                         COMPLETE
E docs/AI routing consistency                            COMPLETE
```

Final controlled repository cleanup / zero-debt sweep также завершён:

```text
SOFTWARE CLEANUP COMPLETE — 100%
DELETE  no remaining proven candidates
MERGE   no duplicate authoritative owners remain
REVIEW  none remain in the named cleanup queue
CI      current Actions confirmed GREEN by operator
```

Не начинать broad cleanup заново без конкретного нового inconsistency, failing test, runtime defect или stale contract evidence.

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
- current linked-production manual writeoff requires exact `source_session_id + source_run_id + immutable spool_id`;
- historical `UNALLOCATED` KG_FIRST remains read/audit/recovery compatibility evidence only, not permission to drop a selected spool from a new run;
- operational cancellation does not erase immutable run/history evidence;
- backup restore operator-only, transactional and fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion/truncation.

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
-> immutable snapshot + exact immutable spool selection -> UART JOB
-> physical START -> RUN_STARTED/RUN_COMPLETED
-> explicit manual exact-run exact-spool writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

## Crash-residue policy already reviewed

```text
JobStateStore .tmp/.bak
  KEEP fail-closed replacement evidence

JobSpoolSelectionStore .json.tmp
  KEEP bounded recovery of one fully valid pre-UART selection temp when final is absent

JobSnapshotStore .json.tmp
  KEEP non-authoritative preparation crash evidence; no auto-promote/resume/delete
```

Do not reopen these merely to make stores symmetrical; their durable transaction boundaries differ.

## External hardware gate

Hardware E2E remains separate from software cleanup completion. For final physical release confidence, targeted two-board ESP32<->Arduino UART/repeat/cancel/reboot smoke should be performed when needed. Hardware GREEN is never inferred from CI.

Do not ask for broad Serial logs. Ask for an exact capture window only when a concrete unresolved issue becomes hardware-only.

## Next work rule

Software cleanup is closed. Continue from the active winding-reference integration log or another concrete product/runtime goal, hardware verification result, bug, feature, or documentation contract change. Do not restart completed audit/provenance/crash-residue work without new evidence.
