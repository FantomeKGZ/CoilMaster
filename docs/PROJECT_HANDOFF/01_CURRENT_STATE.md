# Текущее состояние CoilMaster

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл описывает только **текущее** состояние. Исторические детали находятся в numbered checkpoints и не являются очередью активных работ.

## Source of truth

Единственный источник реализации — `cmp-protocol-v1`. `main` не использовать как источник кода.

Перед изменением existing file fetch текущего содержимого и blob SHA. Не объявлять build/CI/hardware success без фактического результата.

## Last verified production baseline

Production commit:

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
Fix duplicate ESP32 job lifecycle definitions
```

Verified GitHub Actions на exact commit:

```text
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

Пользователь позже сообщил, что последующие workflows также зелёные, но текущий production candidate ниже ещё требует exact automated result после нового safety change.

## Current production candidate

```text
ce38711ca9ecd21fa3432e0981f0933878ac85dd
Keep timed-out jobs in manual review
```

`CM_JobStateDismiss.cpp` больше не трактует `JobDeliveryState::TimedOut` как обычный terminal delivery. Потеря всех `JOB_ACK` остаётся неоднозначной: Arduino могла принять JOB, поэтому timeout разрешается только explicit manual-review path.

Regression guard:

```text
Tests/Web/check_job_cancel_recovery_contracts.js
fa5a0073ee80c58a1d1cab67d50a1249b9479d00
```

Current candidate status:

```text
ESP32 Build — NOT VERIFIED after ce38711c
CMP Protocol Tests — NOT VERIFIED after ce38711c/fa5a0073
```

## Architectural ownership

Arduino Uno:

- physical START;
- SSR;
- Hall turn counting;
- keypad/LCD/buzzer;
- realtime winding state machine;
- accepted remote job execution;
- RUN_STARTED/RUN_COMPLETED generation.

ESP32:

- Wi-Fi/AP/HTTP/FTP;
- microSD/RTC;
- workshop registry;
- winding job preparation/persistence;
- warehouse/materials/costing;
- winding journal/archive;
- backup/restore;
- mobile/desktop UI.

Production cross-board protocol: text `CMP1|...`. `Shared/Protocol/` is older binary host-test code, not production wire protocol.

## Safety boundary

- physical START только local/physical;
- no automatic START between repeats;
- no auto-resume after reboot;
- ESP32/Web never drive SSR directly;
- lost ACK / `TIMED_OUT` never proves Arduino idle;
- `RUN_COMPLETED` never auto-writes off material;
- manual writeoff requires exact `source_session_id + source_run_id`;
- `spool_id` optional only for approved KG_FIRST unallocated/manual consumption;
- exact spool provenance remains mandatory whenever a spool is actually used;
- backup restore operator-only, transactional, fail-closed;
- hardware settings and Hall calibration safe-idle gated.

## Current production flow

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> immutable job snapshot/material selection
-> UART delivery -> physical START
-> RUN_STARTED / RUN_COMPLETED
-> manual exact-run material writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

A repeat target is one JOB containing multiple physical RUNs. Every RUN requires a new physical START.

## JOB cancel / recovery

Implemented:

- no-run remote JOB cancel;
- lost-ACK cancel escalates to remote `JOB_CANCEL` rather than local-only closure;
- Arduino idempotent `ALREADY_CLEAR` when remote job is absent;
- physical fallback `D -> * -> # -> D`;
- fallback emits `ALL_CLEAR`, never `RUN_COMPLETED`;
- active physical run prevents unsafe clear;
- positive remote cancel closes persisted state only with zero physical-run evidence;
- `TIMED_OUT` requires manual review and cannot use ordinary inactive dismiss;
- recovery is re-evaluated before a new job may be created;
- reboot never auto-starts and never fabricates completion/writeoff;
- immutable snapshots/history are not erased by operational cancellation.

## Material/writeoff model

Current new-consumption model is KG_FIRST:

```text
quantity_kg authoritative
exact source_session_id + source_run_id provenance mandatory
spool optional only for unallocated/manual KG_FIRST path
exact spool decrement only with explicit spool
legacy exact-spool records remain supported
```

Wire/material writeoff remains explicit operator action after a completed run.

## Persistence and integrity

Implemented persisted domains include:

- clients/motors/repairs/status;
- winding job allocator/snapshots/state/spool selection;
- winding event journal;
- warehouse/spools/movements/pricing;
- materials usage/adjustments;
- autonomous Arduino winding archive;
- conductor settings;
- backup/restore metadata.

Backup deep validation is read-only and fail-closed. Session persistence preflight is owned by `WindingSessionPersistenceIntegrityAudit`; duplicate full session-directory preflight in backup export was removed.

## UI/API state

Desktop and mobile interfaces cover the implemented workshop, motor/import, repairs, linked winding, Arduino archive, materials/warehouse, costing, reports, settings, backup/network and diagnostics flows. Growing collections use bounded/paged APIs where implemented; old checkpoints describing their migration are historical, not active work.

## Verification still separate

Current safety candidate needs its exact automated gates. Hardware verification is separate from CI:

```text
ESP32 Build current candidate
CMP Protocol Tests current candidate
two-board UART hardware smoke current candidate
full hardware acceptance only when affected scope requires it
```

## Current active direction

1. verify `ce38711c`/`fa5a0073` through ESP32 Build + CMP Protocol Tests;
2. perform targeted ESP32<->Arduino UART/cancel smoke when hardware is available;
3. fix only concrete current failures;
4. use measured data for performance/storage changes;
5. otherwise continue with requested product work.

Do **not** restart old archive/pagination/backup/KG_FIRST work merely because historical checkpoints contain old `next` sections.

Current active queue:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```
