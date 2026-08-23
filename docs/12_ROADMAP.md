# CoilMaster — дорожная карта

## Статус

Исторические этапы архитектуры, Arduino Core, CMP1 integration, ESP32 persistence/API, desktop/mobile Web, warehouse/materials/costing, backup/restore и controlled cleanup реализованы в `cmp-protocol-v1`.

Этот файл больше не является списком незавершённых v0.x задач. Активная очередь и verified GREEN baseline всегда берутся из:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

## Завершённые software blocks

- Arduino realtime state machine, Hall, physical START, SSR, keypad/LCD/buzzer;
- production CMP1 UART JOB/ACK/cancel/run-event transport;
- lost-ACK/timeout and no-run recovery semantics;
- ESP32 microSD/RTC/network/Web/API orchestration;
- clients/motors/repairs and motor import/reference workflows;
- immutable winding job snapshot/state/exact spool selection;
- winding journal and autonomous local winding archive;
- wire spool warehouse, auxiliary materials, pricing/costing and manual exact-run writeoff;
- repair finalization preflight and persisted CLOSED evidence;
- desktop/mobile/shared Web UI and reference site;
- deep persistence/integrity audit, backup/export/restore/rollback safety;
- CI build/test routing and extensive source/static contract regression suite;
- full repo audit A–E and controlled zero-debt cleanup.

## Current release path

### 1. Controlled software cleanup — COMPLETE

Software cleanup checkpoint закрыт на 100% по текущей build-included очереди. Неразрешённых доказанных `DELETE / MERGE / REVIEW` кандидатов нет.

```text
DELETE  no remaining proven cleanup candidates
MERGE   no duplicate authoritative owners remain
KEEP    reviewed live production/build/test/docs/recovery owners
REVIEW  none remain in the named cleanup queue
```

Не начинать новый broad zero-debt sweep без конкретного нового source inconsistency, failing test, runtime defect или stale contract evidence.

### 2. Keep applicable CI GREEN

Relevant gates include:

```text
Arduino Uno Build
ESP32 Build
CMP Protocol Tests
Motor reference index
```

Текущий cleanup implementation/test batch подтверждён GREEN оператором 2026-08-23; точный актуальный baseline ведётся в `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

Новый firmware/runtime commit считается GREEN только после фактического успешного applicable run. Documentation-only commit сам по себе не устанавливает новый firmware GREEN baseline.

### 3. Targeted two-board hardware acceptance

Final physical release confidence remains separate from software cleanup. Required smoke should cover the currently relevant physical boundaries, including where applicable:

- ESP32 -> Arduino JOB delivery and `JOB_ACK`;
- physical START only on Arduino;
- `RUN_STARTED` / `RUN_COMPLETED` propagation and event ACK/retry;
- repeat behavior with no automatic physical START;
- no-run cancel / lost-ACK recovery / `ALL_CLEAR`;
- ESP32 reboot with persisted uncertainty and no auto-resume;
- Hall calibration/telemetry safety;
- manual exact-run exact-spool writeoff after confirmed completed run;
- backup/restore recovery scenarios only on safe/disposable test media where destructive fault injection is required.

Runtime/Serial capture is requested only for an unresolved hardware-only question, not as a routine source-maintenance step.

### 4. Next product/runtime development

После закрытия cleanup новый software backlog формируется только от конкретной продуктовой или runtime-задачи. Приоритет — не повторный общий аудит, а расширение либо исправление production flow с сохранением текущих safety/provenance контрактов.

Каждая новая задача должна:

- использовать `cmp-protocol-v1` как единственный source-of-truth;
- сохранять immutable session/run/spool provenance;
- не переносить physical START/SSR ownership с Arduino;
- не вводить automatic resume или automatic material writeoff;
- добавлять regression/contract coverage там, где меняется проверяемое поведение;
- синхронизировать `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md` после существенного изменения состояния проекта.

## Stable safety acceptance criteria

The project must never regress these invariants:

- no automatic physical START;
- no automatic START between repeat runs;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- no automatic winding resume after reboot;
- lost ACK/timeout does not prove Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never performs automatic wire/material writeoff;
- current linked writeoff requires exact `source_session_id + source_run_id + immutable spool_id`;
- cancellation does not erase immutable run/history evidence;
- restore is explicit/operator-only, transactional and fail-closed;
- reboot does not auto-continue restore/apply;
- production data/evidence is never automatically deleted or truncated as a space/recovery shortcut.

## Cleanup completion definition — satisfied

Software cleanup checkpoint was closed after:

1. remaining owners were classified and no proven duplicate/orphan implementation remained;
2. current high-level/AI/handoff docs were aligned with production source paths/contracts;
3. build/test workflow routing matched `cmp-protocol-v1` source ownership;
4. applicable source/static regressions passed according to the recorded verification checkpoint;
5. the final cleanup checkpoint contained no hidden unresolved named `REVIEW` items.

Hardware acceptance remains an explicit subsequent/parallel release gate and is never inferred from CI.
