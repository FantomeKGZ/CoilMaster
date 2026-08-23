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

Последний подтверждённый implementation/test GREEN baseline:

```text
ad17bb7029f9f0f694fcb275ce729d0c23c8e1dd
Harden release safety contract against stale JSON assertions
CMP Protocol Tests GREEN
```

Точные Actions evidence записаны в `06_ACTIVE_WORK_AND_NEXT_STEPS.md`. Более поздние documentation-only commits не создают новый firmware GREEN baseline. Более поздние implementation changes требуют нового applicable workflow result или явного подтверждения пользователя.

## Current active phase

Full code audit A..E завершён:

```text
A Arduino safety/realtime/UART/resources                  COMPLETE
B ESP32 runtime/API/persistence/integrity/network/backup COMPLETE
C desktop/mobile/shared Web parity/error/security        COMPLETE
D tests/CI/build filters/triggers                         COMPLETE
E docs/AI routing consistency                            COMPLETE
```

Authoritative audit checkpoint:

```text
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
```

Текущая software phase — **final controlled repository cleanup / zero-debt sweep**. Актуальная оценка и остаток всегда берутся из:

```text
docs/PROJECT_HANDOFF/67_NEXT_CHAT_HANDOFF_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

На момент этого обновления cleanup оценивается примерно в **96% complete / 4% remaining**. Не переносить этот процент механически после новых изменений.

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
- a future true unallocated production workflow, if ever needed, must establish immutable material provenance before UART rather than as a post-run fallback;
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

Do not reopen this as generic cleanup merely to make all stores look alike:

```text
JobStateStore .tmp/.bak
  KEEP fail-closed replacement evidence

JobSpoolSelectionStore .json.tmp
  KEEP bounded recovery of one fully valid pre-UART selection temp when final is absent

JobSnapshotStore .json.tmp
  REVIEW / fail-closed resilience; before durable state, no ad-hoc auto-delete/promote
```

The policies differ because their durable transaction boundaries differ.

## External hardware gate

Targeted two-board ESP32<->Arduino UART/repeat/cancel/reboot smoke remains required for final physical release confidence when the stand is needed. It is separate from software cleanup completion. Hardware GREEN is never inferred from CI.

Do not ask for broad Serial logs during source cleanup. Ask for an exact capture window only when an unresolved issue becomes hardware-only.

## Cleanup phase rule

Continue final cleanup directly. For every candidate prove dependency ownership first and classify:

```text
DELETE / MERGE / KEEP / REVIEW
```

Inspect build inclusion, direct call-sites, HTTP/static ownership, persistence/recovery, tests, operator workflow and docs routing. A filename containing `Legacy`, `Old` or similar is never deletion proof.

Current remaining work is primarily final owner inventory, stale thematic contract/docs sweep, final tree/Web/shared/scripts/tools zero-debt pass and handoff consolidation. Do not restart completed A–E audit or provenance/residue review without a concrete current-source inconsistency.
